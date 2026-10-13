package assets

import (
	"encoding/json"
	"fmt"
	"io"
	"os"
	"path/filepath"

	"github.com/zeebo/xxh3"
)

const indexVersion = 1

// IndexEntry records an artifact Forge produced. Its presence is what makes
// Forge willing to delete a file from data/core: anything not recorded here
// was placed by hand and is never touched.
type IndexEntry struct {
	Descriptor string `json:"descriptor"`
	Key        string `json:"key"`
	OutputHash string `json:"outputHash"`
	Size       int64  `json:"size"`
}

// Index maps a destination VFS path to the artifact Forge wrote there.
type Index struct {
	Version int                   `json:"version"`
	Entries map[string]IndexEntry `json:"entries"`
}

// Cache stores compiled artifacts keyed by the hash of everything that went
// into producing them.
type Cache struct {
	dir     string
	objects string
	index   Index
}

// OpenCache loads the cache from dir, creating it if needed. An index written
// by an incompatible version is discarded rather than misinterpreted.
func OpenCache(dir string) (*Cache, error) {
	cache := &Cache{
		dir:     dir,
		objects: filepath.Join(dir, "objects"),
		index: Index{
			Version: indexVersion,
			Entries: map[string]IndexEntry{},
		},
	}

	if err := os.MkdirAll(cache.objects, 0755); err != nil {
		return nil, fmt.Errorf("failed to create cache directory: %w", err)
	}

	data, err := os.ReadFile(cache.indexPath())
	if os.IsNotExist(err) {
		return cache, nil
	} else if err != nil {
		return nil, fmt.Errorf("failed to read cache index: %w", err)
	}

	var loaded Index
	if err := json.Unmarshal(data, &loaded); err != nil {
		// A corrupt index only costs a rebuild, so it is not worth failing over.
		fmt.Printf("Warning: cache index is unreadable (%v), rebuilding.\n", err)
		return cache, nil
	}

	if loaded.Version == indexVersion && loaded.Entries != nil {
		cache.index = loaded
	}

	return cache, nil
}

func (c *Cache) indexPath() string {
	return filepath.Join(c.dir, "index.json")
}

// ObjectPath returns where the artifact for a cache key is stored.
func (c *Cache) ObjectPath(key string) string {
	return filepath.Join(c.objects, key)
}

// HasObject reports whether a compiled artifact for this key already exists.
func (c *Cache) HasObject(key string) bool {
	info, err := os.Stat(c.ObjectPath(key))
	return err == nil && !info.IsDir()
}

// Lookup returns the entry recorded for a destination.
func (c *Cache) Lookup(destination string) (IndexEntry, bool) {
	entry, exists := c.index.Entries[destination]
	return entry, exists
}

// Record stores the entry for a destination.
func (c *Cache) Record(destination string, entry IndexEntry) {
	c.index.Entries[destination] = entry
}

// Forget drops the entry for a destination.
func (c *Cache) Forget(destination string) {
	delete(c.index.Entries, destination)
}

// Destinations lists every destination the index knows about.
func (c *Cache) Destinations() []string {
	destinations := make([]string, 0, len(c.index.Entries))
	for destination := range c.index.Entries {
		destinations = append(destinations, destination)
	}

	return destinations
}

// StoreObject moves a freshly compiled file into the cache. The move is a
// rename so that an interrupted run cannot leave a truncated artifact behind
// that a later run would trust.
func (c *Cache) StoreObject(key, sourcePath string) error {
	return moveFile(sourcePath, c.ObjectPath(key))
}

// Save writes the index to disk.
func (c *Cache) Save() error {
	c.index.Version = indexVersion

	data, err := json.MarshalIndent(c.index, "", "  ")
	if err != nil {
		return fmt.Errorf("failed to encode cache index: %w", err)
	}

	return writeFileAtomic(c.indexPath(), data)
}

// TempFile creates a file in the cache directory for a tool to write into.
// It lives on the same filesystem as the object store, so promoting it to an
// object is a rename rather than a copy.
func (c *Cache) TempFile(pattern string) (string, error) {
	file, err := os.CreateTemp(c.dir, "tmp-"+pattern)
	if err != nil {
		return "", fmt.Errorf("failed to create temporary file: %w", err)
	}
	name := file.Name()
	file.Close()

	return name, nil
}

// HashFile returns the xxh3-128 of a file's contents, hex encoded. xxh3 is
// already what the manifest and the web VFS agree on.
func HashFile(path string) (string, error) {
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

// CopyFile copies src to dst, creating parent directories as needed. The copy
// goes through a temporary file so an interrupted run cannot leave a partial
// asset in data/core.
func CopyFile(src, dst string) error {
	if err := os.MkdirAll(filepath.Dir(dst), 0755); err != nil {
		return fmt.Errorf("failed to create directory for %s: %w", dst, err)
	}

	data, err := os.ReadFile(src)
	if err != nil {
		return fmt.Errorf("failed to read %s: %w", src, err)
	}

	return writeFileAtomic(dst, data)
}

func writeFileAtomic(path string, data []byte) error {
	if err := os.MkdirAll(filepath.Dir(path), 0755); err != nil {
		return fmt.Errorf("failed to create directory for %s: %w", path, err)
	}

	temp, err := os.CreateTemp(filepath.Dir(path), "."+filepath.Base(path)+".tmp")
	if err != nil {
		return fmt.Errorf("failed to create temporary file for %s: %w", path, err)
	}
	tempName := temp.Name()

	if _, err := temp.Write(data); err != nil {
		temp.Close()
		os.Remove(tempName)
		return fmt.Errorf("failed to write %s: %w", path, err)
	}

	if err := temp.Close(); err != nil {
		os.Remove(tempName)
		return fmt.Errorf("failed to write %s: %w", path, err)
	}

	if err := os.Rename(tempName, path); err != nil {
		os.Remove(tempName)
		return fmt.Errorf("failed to move %s into place: %w", path, err)
	}

	return nil
}

// moveFile renames src to dst, falling back to a copy if the two are on
// different filesystems.
func moveFile(src, dst string) error {
	if err := os.MkdirAll(filepath.Dir(dst), 0755); err != nil {
		return fmt.Errorf("failed to create directory for %s: %w", dst, err)
	}

	if err := os.Rename(src, dst); err == nil {
		return nil
	}

	if err := CopyFile(src, dst); err != nil {
		return err
	}

	return os.Remove(src)
}
