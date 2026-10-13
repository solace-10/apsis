package assets

import (
	"bytes"
	"fmt"
	"image"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"sync"

	// Registers the decoders used to read source dimensions.
	_ "image/jpeg"
	_ "image/png"

	"wings_of_steel/forge/paths"
)

func init() {
	Register(&textureTool{}, ".png", ".jpg", ".jpeg")
}

// ktxVendorDocs points at the instructions for adding a platform's encoder.
const ktxVendorDocs = "pandora/tools/forge/bin/ktx/README.md"

// textureOptions are the descriptor fields the texture tool understands.
type textureOptions struct {
	// Width and Height resize the source. Giving only one of them preserves
	// the aspect ratio.
	Width  int `json:"width"`
	Height int `json:"height"`

	// Usage decides how many channels the texture keeps, the colour space it is
	// encoded in, and the format it ends up in. Textures that carry data rather
	// than colour must be linear, or their values are gamma-mangled on load.
	Usage string `json:"usage"`

	// Encoding deviates from the block compression a four channel usage would
	// otherwise get. Leaving it out is almost always right; see the constants
	// below for the two reasons to set it. The one and two channel usages have a
	// fixed output format and reject it outright.
	Encoding string `json:"encoding"`
}

const (
	usageColor  = "color"
	usageNormal = "normal"
	usageLinear = "linear"
	usageMask   = "mask"
)

const (
	// encodingUASTC is the default for a four channel texture, and what the one
	// and two channel usages pass through on their way to a block format.
	encodingUASTC = "uastc"

	// encodingBasisLZ trades quality for size, and only makes sense where the
	// download matters more than the texture does - background detail, decals,
	// anything the eye does not rest on. Measured on this project's colour map it
	// is eight times smaller than UASTC and 15 dB worse.
	encodingBasisLZ = "basis-lz"

	// encodingNone ships plain RGBA8, which costs four times the memory on the
	// GPU. It is right only where block compression artifacts would show and the
	// texture is small enough not to care: UI, gradients, lookup tables.
	encodingNone = "none"
)

// zstdLevel is the supercompression level every artifact that can take one gets.
//
// It is not a per-asset decision. The level costs encode time and almost nothing
// else - decompression runs at gigabytes per second whatever it was compressed
// at - and encode time is paid once, by the cache. 19 rather than 22 because 20
// and above are zstd's "ultra" tier, which widens the decompression window and
// so the memory a WASM heap has to find, in exchange for 0.2% on these textures.
const zstdLevel = 19

// texturePipeline is how a usage becomes ktx invocations. Both Compile and
// Fingerprint go through it, so they cannot disagree about what a usage means.
type texturePipeline struct {
	// createFormat is the --format ktx create loads the source into. It is what
	// decides how many channels survive: a single channel mask loaded as
	// R8G8B8A8 would be encoded, shipped and uploaded as four.
	createFormat string

	// transferFunction is the --assign-tf label.
	transferFunction string

	// normalMode passes --normal-mode, which rewrites a three component normal
	// map into the two component (RGB=X, A=Y) layout Basis encodes, discarding
	// Z. The engine reconstructs it as sqrt(1 - x*x - y*y).
	normalMode bool

	// transcodeTarget, when set, is the block format the artifact is transcoded
	// into before shipping, leaving no Basis payload behind. Basis is the wrong
	// transport for one and two channel data: UASTC is structurally four channel
	// and spends eight bits per texel whatever it is given, so it cannot even
	// represent a mask more cheaply than a colour map.
	transcodeTarget string

	// encoding is what --encode receives once the usage's default has been
	// applied. Empty ships plain texels.
	encoding string

	// zstd is the supercompression level, or zero for the BasisLZ case, where
	// ktx refuses one because the payload already carries its own.
	zstd int
}

func pipelineFor(options textureOptions) texturePipeline {
	pipeline := texturePipeline{zstd: zstdLevel}

	switch options.Usage {
	case usageMask:
		// BC4 is half the memory of BC7 and gives the one channel it keeps the
		// whole 4x4 block to itself, rather than sharing endpoints with RGB.
		pipeline.createFormat = "R8_UNORM"
		pipeline.transferFunction = "linear"
		pipeline.transcodeTarget = "bc4"
		pipeline.encoding = encodingUASTC

	case usageNormal:
		// BC5 is two independent BC4 channels, which is both better precision for
		// X and Y than BC7 can give them and the only target whose result reads
		// back as .rg - BC7 would put X in RGB and Y in alpha.
		pipeline.createFormat = "R8G8B8A8_UNORM"
		pipeline.transferFunction = "linear"
		pipeline.normalMode = true
		pipeline.transcodeTarget = "bc5"
		pipeline.encoding = encodingUASTC

	case usageColor:
		pipeline.createFormat = "R8G8B8A8_SRGB"
		pipeline.transferFunction = "srgb"
		pipeline.encoding = resolveEncoding(options.Encoding)

	default:
		pipeline.createFormat = "R8G8B8A8_UNORM"
		pipeline.transferFunction = "linear"
		pipeline.encoding = resolveEncoding(options.Encoding)
	}

	// ktx rejects the combination outright: BasisLZ brings its own entropy coding
	// and there is nothing left for zstd to find.
	if pipeline.encoding == encodingBasisLZ {
		pipeline.zstd = 0
	}

	return pipeline
}

// resolveEncoding applies the default an omitted encoding stands for. Shipping
// uncompressed is never what someone who left the field out meant, so the
// default is the one worth shipping and 'none' has to be asked for.
func resolveEncoding(encoding string) string {
	switch encoding {
	case "":
		return encodingUASTC
	case encodingNone:
		return ""
	default:
		return encoding
	}
}

type textureTool struct{}

func (t *textureTool) Name() string {
	return "texture"
}

func (t *textureTool) OutputExtension(srcExt string) string {
	return ".ktx2"
}

func (t *textureTool) Fingerprint(d *Descriptor) (string, error) {
	options, err := parseTextureOptions(d)
	if err != nil {
		return "", err
	}

	// The encoder version is part of the fingerprint so that upgrading
	// KTX-Software invalidates every texture, rather than leaving a mix of
	// old and new artifacts in the cache.
	version, err := ktxVersion()
	if err != nil {
		return "", err
	}

	// The resolved pipeline is fingerprinted rather than the raw options, so that
	// changing a default - or the zstd level - invalidates the cache by itself.
	//
	// The version prefix moved to 2 when usage started deciding the channel count
	// and output format, and to 3 when the encoding gained a default. Without it
	// an unchanged descriptor would fingerprint identically while producing a
	// different artifact, and every cached texture would be wrongly restored.
	pipeline := pipelineFor(options)

	return fmt.Sprintf("texture/3 %s w=%d h=%d usage=%s encoding=%s zstd=%d",
		version, options.Width, options.Height, options.Usage, pipeline.encoding, pipeline.zstd), nil
}

func (t *textureTool) Compile(job *Job) error {
	options, err := parseTextureOptions(job.Descriptor)
	if err != nil {
		return err
	}

	executable, err := ktxExecutable()
	if err != nil {
		return err
	}

	pipeline := pipelineFor(options)

	// A usage with a transcode target ships the block format itself, which takes
	// two passes: ktx create cannot write a BC format directly, but ktx transcode
	// can. The intermediate exists only to give the transcode something to read
	// and never leaves this function.
	createOutput := job.OutputPath
	if pipeline.transcodeTarget != "" {
		intermediate, err := os.CreateTemp(filepath.Dir(job.OutputPath), "basis-*.ktx2")
		if err != nil {
			return fmt.Errorf("failed to create the intermediate texture: %w", err)
		}
		intermediate.Close()
		defer os.Remove(intermediate.Name())
		createOutput = intermediate.Name()
	}

	// --assign-tf labels the transfer function without touching the samples,
	// which is what is wanted here: the sources are already sRGB encoded, so a
	// linear usage is a statement that the bytes are data rather than colour
	// and must not be gamma decoded.
	arguments := []string{"create", "--generate-mipmap",
		"--format", pipeline.createFormat,
		"--assign-tf", pipeline.transferFunction}

	if pipeline.normalMode {
		// --normalize is deliberately not passed alongside this. It computes 2D
		// unit normals, which would rescale a shallow relief map - where X and Y
		// stay near zero because the relief is tiny next to the radius - until
		// every texel claimed a 90 degree slope.
		arguments = append(arguments, "--normal-mode")
	}

	// ktx resamples to whichever dimensions are given, but it does not infer
	// the missing one, so the aspect ratio is worked out here.
	if options.Width > 0 || options.Height > 0 {
		width, height, err := targetSize(job.SourcePath, options)
		if err != nil {
			return err
		}
		arguments = append(arguments, "--width", strconv.Itoa(width), "--height", strconv.Itoa(height))
	}

	if pipeline.encoding != "" {
		// UASTC rather than basis-lz for a transcode target, because the step out of
		// it into a block format is near-lossless where ETC1S would bake its own
		// losses in first.
		arguments = append(arguments, "--encode", pipeline.encoding)
	}

	// Supercompression waits for the transcode when there is one, as the
	// intermediate is about to be thrown away.
	if pipeline.zstd > 0 && pipeline.transcodeTarget == "" {
		arguments = append(arguments, "--zstd", strconv.Itoa(pipeline.zstd))
	}

	arguments = append(arguments, job.SourcePath, createOutput)

	if err := runKtx(executable, arguments); err != nil {
		return err
	}

	if pipeline.transcodeTarget == "" {
		return nil
	}

	transcode := []string{"transcode", "--target", pipeline.transcodeTarget}
	if pipeline.zstd > 0 {
		transcode = append(transcode, "--zstd", strconv.Itoa(pipeline.zstd))
	}
	transcode = append(transcode, createOutput, job.OutputPath)

	return runKtx(executable, transcode)
}

// runKtx invokes the vendored encoder, reporting whatever it printed rather than
// just an exit status.
func runKtx(executable string, arguments []string) error {
	command := exec.Command(executable, arguments...)
	var stderr bytes.Buffer
	command.Stderr = &stderr

	if err := command.Run(); err != nil {
		message := strings.TrimSpace(stderr.String())
		if message == "" {
			message = err.Error()
		}
		return fmt.Errorf("ktx %s failed: %s", arguments[0], message)
	}

	return nil
}

func parseTextureOptions(d *Descriptor) (textureOptions, error) {
	options := textureOptions{Usage: usageColor}
	if err := d.DecodeOptions(&options); err != nil {
		return options, err
	}

	switch options.Usage {
	case usageColor, usageNormal, usageLinear, usageMask:
	default:
		return options, fmt.Errorf("unknown usage '%s' (expected '%s', '%s', '%s' or '%s')",
			options.Usage, usageColor, usageNormal, usageLinear, usageMask)
	}

	switch options.Encoding {
	case "", encodingNone, encodingUASTC, encodingBasisLZ:
	default:
		return options, fmt.Errorf("unknown encoding '%s' (expected '%s', '%s' or '%s')",
			options.Encoding, encodingNone, encodingUASTC, encodingBasisLZ)
	}

	// A usage with a fixed output format leaves 'encoding' nothing to decide, so
	// accepting one would only invite the belief that it did something.
	if target := pipelineFor(options).transcodeTarget; target != "" && options.Encoding != "" {
		return options, fmt.Errorf("usage '%s' does not take an 'encoding': it always produces %s",
			options.Usage, strings.ToUpper(target))
	}

	if options.Width < 0 || options.Height < 0 {
		return options, fmt.Errorf("width and height must not be negative")
	}

	return options, nil
}

// targetSize resolves the requested dimensions, reading the source's own size
// to fill in whichever one was left out.
func targetSize(sourcePath string, options textureOptions) (int, int, error) {
	if options.Width > 0 && options.Height > 0 {
		return options.Width, options.Height, nil
	}

	file, err := os.Open(sourcePath)
	if err != nil {
		return 0, 0, fmt.Errorf("failed to read the source image: %w", err)
	}
	defer file.Close()

	config, _, err := image.DecodeConfig(file)
	if err != nil {
		return 0, 0, fmt.Errorf("failed to read the source image dimensions: %w", err)
	}

	width := options.Width
	height := options.Height

	if width > 0 {
		height = config.Height * width / config.Width
	} else {
		width = config.Width * height / config.Height
	}

	if width < 1 {
		width = 1
	}
	if height < 1 {
		height = 1
	}

	return width, height, nil
}

var (
	ktxOnce        sync.Once
	ktxPath        string
	ktxPathErr     error
	ktxVersionOnce sync.Once
	ktxVersionText string
	ktxVersionErr  error
)

// ktxExecutable returns the encoder vendored alongside Forge for this
// platform.
//
// It is the only encoder Forge will use. Textures are then guaranteed to be
// encoded identically everywhere, rather than depending on what each machine
// happens to have installed.
func ktxExecutable() (string, error) {
	ktxOnce.Do(func() {
		binDir, err := paths.ForgeBinDir()
		if err != nil {
			ktxPathErr = err
			return
		}

		name := "ktx"
		if runtime.GOOS == "windows" {
			name = "ktx.exe"
		}

		path := filepath.Join(binDir, "ktx", runtime.GOOS+"-"+runtime.GOARCH, name)
		if _, err := os.Stat(path); err != nil {
			ktxPathErr = fmt.Errorf("no ktx encoder is vendored for %s/%s: expected it at %s (see %s)",
				runtime.GOOS, runtime.GOARCH, path, ktxVendorDocs)
			return
		}

		ktxPath = path
	})

	return ktxPath, ktxPathErr
}

func ktxVersion() (string, error) {
	ktxVersionOnce.Do(func() {
		executable, err := ktxExecutable()
		if err != nil {
			ktxVersionErr = err
			return
		}

		output, err := exec.Command(executable, "--version").CombinedOutput()
		if err != nil {
			ktxVersionErr = fmt.Errorf("failed to query the ktx version: %w", err)
			return
		}

		ktxVersionText = strings.TrimSpace(string(output))
	})

	return ktxVersionText, ktxVersionErr
}
