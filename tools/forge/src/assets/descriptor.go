package assets

import (
	"bytes"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
)

// DescriptorSuffix identifies an asset descriptor. Plain ".json" is not used
// because a source asset may itself be a JSON file, and the two must remain
// distinguishable.
const DescriptorSuffix = ".asset.json"

// commonFields are the descriptor keys handled by the compiler itself. Every
// other key is passed through to the tool as its options.
var commonFields = map[string]bool{
	"source":      true,
	"destination": true,
	"tool":        true,
}

// Descriptor is a single parsed asset descriptor.
type Descriptor struct {
	// DescriptorPath is the absolute path of the descriptor file.
	DescriptorPath string
	// RelPath is the descriptor's path relative to the assets directory, used
	// in messages and stored in the cache index.
	RelPath string

	// Source is the input file, relative to the assets directory.
	Source string
	// Destination is the output file, relative to the data/core directory.
	Destination string
	// Tool optionally overrides extension-based tool selection.
	Tool string

	// SourcePath and OutputPath are the resolved absolute paths.
	SourcePath string
	OutputPath string

	// Options holds the tool-specific fields as canonical JSON. Keys are
	// sorted by encoding/json, so the encoding is stable enough to hash.
	Options []byte
}

// VFSPath returns the destination as the game sees it through the VFS, which
// is also the key used by the manifest and the cache index.
func (d *Descriptor) VFSPath() string {
	return "/" + filepath.ToSlash(d.Destination)
}

// DecodeOptions unmarshals the tool-specific fields into out, rejecting any
// field the tool does not recognise. A typo'd option should fail the build
// rather than silently ship a wrongly compiled asset.
//
// Errors are returned without the descriptor's path, as the compiler adds it
// when it reports the failure.
func (d *Descriptor) DecodeOptions(out any) error {
	decoder := json.NewDecoder(bytes.NewReader(d.Options))
	decoder.DisallowUnknownFields()
	if err := decoder.Decode(out); err != nil {
		return err
	}

	return nil
}

// Discover walks assetsDir and parses every descriptor it finds. Results are
// sorted by descriptor path so that runs are reproducible.
func Discover(assetsDir, dataCoreDir string) ([]*Descriptor, error) {
	var descriptors []*Descriptor

	err := filepath.Walk(assetsDir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		if info.IsDir() || !strings.HasSuffix(info.Name(), DescriptorSuffix) {
			return nil
		}

		descriptor, err := parseDescriptor(path, assetsDir, dataCoreDir)
		if err != nil {
			return err
		}

		descriptors = append(descriptors, descriptor)
		return nil
	})

	if err != nil {
		return nil, err
	}

	sort.Slice(descriptors, func(i, j int) bool {
		return descriptors[i].RelPath < descriptors[j].RelPath
	})

	if err := checkForDuplicates(descriptors); err != nil {
		return nil, err
	}

	return descriptors, nil
}

func parseDescriptor(path, assetsDir, dataCoreDir string) (*Descriptor, error) {
	relPath, err := filepath.Rel(assetsDir, path)
	if err != nil {
		return nil, fmt.Errorf("failed to get relative path for %s: %w", path, err)
	}
	relPath = filepath.ToSlash(relPath)

	data, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("failed to read %s: %w", relPath, err)
	}

	// Decode twice: once for the fields the compiler owns, and once as a raw
	// map so the remaining fields can be handed to the tool.
	var fields map[string]json.RawMessage
	if err := json.Unmarshal(data, &fields); err != nil {
		return nil, fmt.Errorf("failed to parse %s: %w", relPath, err)
	}

	descriptor := &Descriptor{
		DescriptorPath: path,
		RelPath:        relPath,
	}

	var common struct {
		Source      string `json:"source"`
		Destination string `json:"destination"`
		Tool        string `json:"tool"`
	}
	if err := json.Unmarshal(data, &common); err != nil {
		return nil, fmt.Errorf("failed to parse %s: %w", relPath, err)
	}

	descriptor.Source = common.Source
	descriptor.Destination = common.Destination
	descriptor.Tool = common.Tool

	options := make(map[string]json.RawMessage, len(fields))
	for key, value := range fields {
		if !commonFields[key] {
			options[key] = value
		}
	}

	// json.Marshal sorts map keys, giving a canonical encoding to hash.
	descriptor.Options, err = json.Marshal(options)
	if err != nil {
		return nil, fmt.Errorf("failed to encode options of %s: %w", relPath, err)
	}

	if err := resolve(descriptor, assetsDir, dataCoreDir); err != nil {
		return nil, err
	}

	return descriptor, nil
}

func resolve(d *Descriptor, assetsDir, dataCoreDir string) error {
	if d.Source == "" {
		return fmt.Errorf("%s: 'source' is required", d.RelPath)
	}

	if err := validateRelative(d.RelPath, "source", d.Source); err != nil {
		return err
	}

	d.Source = filepath.ToSlash(filepath.Clean(d.Source))
	d.SourcePath = filepath.Join(assetsDir, filepath.FromSlash(d.Source))

	info, err := os.Stat(d.SourcePath)
	if err != nil {
		return fmt.Errorf("%s: source '%s' cannot be read: %w", d.RelPath, d.Source, err)
	}
	if info.IsDir() {
		return fmt.Errorf("%s: source '%s' is a directory", d.RelPath, d.Source)
	}

	tool, err := ToolFor(d)
	if err != nil {
		return err
	}

	if d.Destination == "" {
		// Default to the source path with the tool's output extension.
		sourceExt := filepath.Ext(d.Source)
		d.Destination = strings.TrimSuffix(d.Source, sourceExt) + tool.OutputExtension(sourceExt)
	} else if err := validateRelative(d.RelPath, "destination", d.Destination); err != nil {
		return err
	}

	d.Destination = filepath.ToSlash(filepath.Clean(d.Destination))
	d.OutputPath = filepath.Join(dataCoreDir, filepath.FromSlash(d.Destination))

	return nil
}

// validateRelative rejects paths that would let a descriptor read or write
// outside of the directory it is confined to.
func validateRelative(descriptorPath, field, value string) error {
	if filepath.IsAbs(value) || strings.HasPrefix(value, "/") {
		return fmt.Errorf("%s: '%s' must be a relative path, got '%s'", descriptorPath, field, value)
	}

	cleaned := filepath.ToSlash(filepath.Clean(value))
	if cleaned == ".." || strings.HasPrefix(cleaned, "../") {
		return fmt.Errorf("%s: '%s' must not escape its root, got '%s'", descriptorPath, field, value)
	}

	return nil
}

// checkForDuplicates rejects two descriptors writing to the same destination,
// which would otherwise be a race whose winner depends on scheduling.
func checkForDuplicates(descriptors []*Descriptor) error {
	seen := make(map[string]string, len(descriptors))
	for _, descriptor := range descriptors {
		destination := descriptor.VFSPath()
		if previous, exists := seen[destination]; exists {
			return fmt.Errorf("%s and %s both write to '%s'", previous, descriptor.RelPath, destination)
		}
		seen[destination] = descriptor.RelPath
	}

	return nil
}
