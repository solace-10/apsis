// Package paths resolves the well-known directories Forge operates on.
//
// Every path is derived from the location of the Forge executable rather than
// the working directory, as Forge is usually invoked by CMake from whichever
// directory the build happens to be in.
package paths

import (
	"fmt"
	"os"
	"path/filepath"
)

// ProjectRoot returns the root of the project checkout.
//
// The Forge binary lives in pandora/tools/forge/bin/, so the root is four
// levels up from it.
func ProjectRoot() (string, error) {
	exePath, err := os.Executable()
	if err != nil {
		return "", fmt.Errorf("failed to get executable path: %w", err)
	}

	root, err := filepath.Abs(filepath.Join(filepath.Dir(exePath), "..", "..", "..", ".."))
	if err != nil {
		return "", fmt.Errorf("failed to get absolute path: %w", err)
	}

	return root, nil
}

// ForgeBinDir returns the directory the Forge executable lives in, which is
// also where the tools vendored alongside it are kept.
func ForgeBinDir() (string, error) {
	exePath, err := os.Executable()
	if err != nil {
		return "", fmt.Errorf("failed to get executable path: %w", err)
	}

	return filepath.Dir(exePath), nil
}

// AssetsDir returns the directory holding asset descriptors and their sources.
func AssetsDir() (string, error) {
	return subdir("game", "assets")
}

// DataCoreDir returns the directory the game loads its data from, and which
// the asset compiler writes into.
func DataCoreDir() (string, error) {
	return subdir("game", "bin", "data", "core")
}

// GameBinDir returns the game's working directory.
func GameBinDir() (string, error) {
	return subdir("game", "bin")
}

// ManifestPath returns the location of the generated asset manifest.
func ManifestPath() (string, error) {
	return subdir("game", "bin", "manifest.json")
}

// CacheDir returns the asset compilation cache. It deliberately sits at the
// project root rather than under build/, so that wiping the build directory
// does not force every asset to be recompiled.
func CacheDir() (string, error) {
	return subdir(".forge-cache")
}

func subdir(elements ...string) (string, error) {
	root, err := ProjectRoot()
	if err != nil {
		return "", err
	}

	return filepath.Join(append([]string{root}, elements...)...), nil
}
