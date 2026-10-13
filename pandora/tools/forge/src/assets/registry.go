package assets

import (
	"fmt"
	"path/filepath"
	"sort"
	"strings"
)

// Job is a single unit of work handed to a tool.
type Job struct {
	Descriptor *Descriptor
	// SourcePath is the absolute path of the input file.
	SourcePath string
	// OutputPath is where the tool must write its result. It is a temporary
	// path; the compiler moves the result into place once the tool succeeds.
	OutputPath string
}

// Tool compiles one kind of source asset.
//
// Tools register themselves for the extensions they handle, so adding a new
// one means adding a file with an init() and nothing else.
type Tool interface {
	// Name is the identifier a descriptor's "tool" field can select.
	Name() string

	// OutputExtension returns the extension of the compiled artifact, given
	// the source's extension. Return srcExt to keep the source extension.
	OutputExtension(srcExt string) string

	// Fingerprint returns a string capturing everything about this tool and
	// this descriptor's options that affects the output. It is mixed into the
	// cache key, so a changed option or an upgraded encoder invalidates the
	// cached artifact. It is also where option validation happens, so that
	// bad options are reported before any work starts.
	Fingerprint(d *Descriptor) (string, error)

	// Compile reads the job's source and writes its output.
	Compile(job *Job) error
}

// FallbackToolName is used for any extension no tool has registered for.
const FallbackToolName = "copy"

var (
	toolsByName      = map[string]Tool{}
	toolNameByExtens = map[string]string{}
)

// Register makes a tool available, both by name and for the given extensions.
// Extensions are given with a leading dot, e.g. ".png".
func Register(tool Tool, extensions ...string) {
	name := tool.Name()
	if _, exists := toolsByName[name]; exists {
		panic(fmt.Sprintf("asset tool '%s' is registered twice", name))
	}
	toolsByName[name] = tool

	for _, extension := range extensions {
		extension = strings.ToLower(extension)
		if previous, exists := toolNameByExtens[extension]; exists {
			panic(fmt.Sprintf("extension '%s' is claimed by both '%s' and '%s'", extension, previous, name))
		}
		toolNameByExtens[extension] = name
	}
}

// ToolFor resolves the tool for a descriptor: an explicit "tool" field wins,
// otherwise the source extension decides, and anything unrecognised falls back
// to a plain copy.
func ToolFor(d *Descriptor) (Tool, error) {
	if d.Tool != "" {
		tool, exists := toolsByName[d.Tool]
		if !exists {
			return nil, fmt.Errorf("%s: unknown tool '%s' (available: %s)", d.RelPath, d.Tool, strings.Join(ToolNames(), ", "))
		}
		return tool, nil
	}

	extension := strings.ToLower(filepath.Ext(d.Source))
	if name, exists := toolNameByExtens[extension]; exists {
		return toolsByName[name], nil
	}

	return toolsByName[FallbackToolName], nil
}

// ToolNames lists the registered tools, for error messages.
func ToolNames() []string {
	names := make([]string, 0, len(toolsByName))
	for name := range toolsByName {
		names = append(names, name)
	}
	sort.Strings(names)

	return names
}
