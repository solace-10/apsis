package manifest

import (
	"encoding/json"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"

	"github.com/zeebo/xxh3"
)

type Entry struct {
	Path string `json:"path"`
	Hash string `json:"hash"`
	Size int64  `json:"size"`
}

type Generator struct {
	sourceDir string
	outputDir string
}

func NewGenerator(sourceDir, outputDir string) *Generator {
	return &Generator{
		sourceDir: sourceDir,
		outputDir: outputDir,
	}
}

func (g *Generator) Generate() error {
	var entries []Entry

	err := filepath.Walk(g.sourceDir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		if info.IsDir() {
			return nil
		}

		// Skip the manifest file itself if it exists in the source
		if strings.HasSuffix(path, "manifest.json") {
			return nil
		}

		relPath, err := filepath.Rel(g.sourceDir, path)
		if err != nil {
			return fmt.Errorf("failed to get relative path for %s: %w", path, err)
		}

		// Convert to forward slashes and ensure it starts with /
		relPath = "/" + filepath.ToSlash(relPath)

		hash, err := computeHash(path)
		if err != nil {
			return fmt.Errorf("failed to compute hash for %s: %w", path, err)
		}

		entry := Entry{
			Path: relPath,
			Hash: hash,
			Size: info.Size(),
		}

		entries = append(entries, entry)
		fmt.Printf("  %s (%d bytes)\n", relPath, info.Size())

		return nil
	})

	if err != nil {
		return fmt.Errorf("failed to walk directory: %w", err)
	}

	// Ensure output directory exists
	if err := os.MkdirAll(g.outputDir, 0755); err != nil {
		return fmt.Errorf("failed to create output directory: %w", err)
	}

	outputPath := filepath.Join(g.outputDir, "manifest.json")

	jsonData, err := json.MarshalIndent(entries, "", "  ")
	if err != nil {
		return fmt.Errorf("failed to marshal manifest: %w", err)
	}

	if err := os.WriteFile(outputPath, jsonData, 0644); err != nil {
		return fmt.Errorf("failed to write manifest: %w", err)
	}

	fmt.Printf("\nManifest written to %s (%d entries)\n", outputPath, len(entries))

	return nil
}

func computeHash(path string) (string, error) {
	file, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer file.Close()

	hasher := xxh3.New()
	if _, err := io.Copy(hasher, file); err != nil {
		return "", err
	}

	return fmt.Sprintf("%016x", hasher.Sum64()), nil
}
