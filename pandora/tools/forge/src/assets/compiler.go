package assets

import (
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"sync"

	"github.com/zeebo/xxh3"
)

// Options configures a compilation run.
type Options struct {
	AssetsDir   string
	DataCoreDir string
	CacheDir    string

	// Force recompiles every asset, ignoring cached artifacts.
	Force bool
	// Jobs is the number of assets compiled concurrently.
	Jobs int
	// Verbose also reports assets that were already up to date.
	Verbose bool
}

// action is what a descriptor needs before its output is up to date.
type action int

const (
	// actionUpToDate means the destination already holds the right bytes.
	actionUpToDate action = iota
	// actionRestore means the artifact is cached and only needs copying out.
	actionRestore
	// actionCompile means the tool has to run.
	actionCompile
)

// Result summarises a run.
type Result struct {
	Compiled int
	Restored int
	UpToDate int
	Removed  int
	Failed   int
}

type plan struct {
	descriptor *Descriptor
	tool       Tool
	key        string
	action     action
	// err records a failure to work out what this asset needs, so that one
	// unusable descriptor - or one missing encoder - does not stop every other
	// asset from being compiled.
	err error
}

// Compile brings data/core in sync with the descriptors in the assets
// directory.
func Compile(options Options) (Result, error) {
	var result Result

	descriptors, err := Discover(options.AssetsDir, options.DataCoreDir)
	if err != nil {
		return result, err
	}

	cache, err := OpenCache(options.CacheDir)
	if err != nil {
		return result, err
	}

	plans := buildPlans(descriptors, cache, options)

	// Removing orphans first keeps a rename of a destination from deleting the
	// file the same run just wrote.
	removed, err := removeOrphans(descriptors, cache, options)
	if err != nil {
		return result, err
	}
	result.Removed = removed

	failures := run(plans, cache, options, &result)

	if err := cache.Save(); err != nil {
		return result, err
	}

	if len(failures) > 0 {
		result.Failed = len(failures)
		for _, failure := range failures {
			fmt.Fprintf(os.Stderr, "Error: %v\n", failure)
		}
		return result, fmt.Errorf("%d of %d assets failed to compile", len(failures), len(descriptors))
	}

	return result, nil
}

// buildPlans resolves the tool and cache key for every descriptor, and decides
// what each one needs. Tool option validation happens here, so bad options are
// reported before any work starts - but as a per-asset failure, so that the
// assets around them still compile.
func buildPlans(descriptors []*Descriptor, cache *Cache, options Options) []*plan {
	plans := make([]*plan, 0, len(descriptors))

	for _, descriptor := range descriptors {
		item := &plan{descriptor: descriptor}
		plans = append(plans, item)

		tool, err := ToolFor(descriptor)
		if err != nil {
			item.err = err
			continue
		}
		item.tool = tool

		key, err := cacheKey(descriptor, tool)
		if err != nil {
			item.err = err
			continue
		}

		item.key = key
		item.action = decide(descriptor, cache, key, options)
	}

	return plans
}

// decide works out the cheapest way to get the destination up to date.
func decide(descriptor *Descriptor, cache *Cache, key string, options Options) action {
	if options.Force {
		return actionCompile
	}

	entry, recorded := cache.Lookup(descriptor.VFSPath())
	if recorded && entry.Key == key {
		// The destination is only trusted if it still holds what was written,
		// so a hand-edited or deleted file is repaired rather than skipped.
		if hash, err := HashFile(descriptor.OutputPath); err == nil && hash == entry.OutputHash {
			return actionUpToDate
		}
	}

	if cache.HasObject(key) {
		return actionRestore
	}

	return actionCompile
}

// run executes the plans across a bounded pool of workers.
func run(plans []*plan, cache *Cache, options Options, result *Result) []error {
	jobs := options.Jobs
	if jobs < 1 {
		jobs = 1
	}
	if jobs > len(plans) {
		jobs = len(plans)
	}

	var (
		mutex     sync.Mutex
		failures  []error
		waitGroup sync.WaitGroup
	)

	queue := make(chan *plan)

	for worker := 0; worker < jobs; worker++ {
		waitGroup.Add(1)
		go func() {
			defer waitGroup.Done()
			for item := range queue {
				entry, err := execute(item, cache, options)

				mutex.Lock()
				switch {
				case err != nil:
					failures = append(failures, fmt.Errorf("%s: %w", item.descriptor.RelPath, err))
				default:
					cache.Record(item.descriptor.VFSPath(), entry)
					switch item.action {
					case actionCompile:
						result.Compiled++
						fmt.Printf("  compiled %s (%s, %d bytes)\n", item.descriptor.VFSPath(), item.tool.Name(), entry.Size)
					case actionRestore:
						result.Restored++
						fmt.Printf("  restored %s (%d bytes)\n", item.descriptor.VFSPath(), entry.Size)
					default:
						result.UpToDate++
						if options.Verbose {
							fmt.Printf("  up to date %s\n", item.descriptor.VFSPath())
						}
					}
				}
				mutex.Unlock()
			}
		}()
	}

	for _, item := range plans {
		queue <- item
	}
	close(queue)
	waitGroup.Wait()

	return failures
}

// execute performs a single plan and returns the index entry describing the
// artifact now sitting at the destination.
func execute(item *plan, cache *Cache, options Options) (IndexEntry, error) {
	if item.err != nil {
		return IndexEntry{}, item.err
	}

	descriptor := item.descriptor

	entry := IndexEntry{
		Descriptor: descriptor.RelPath,
		Key:        item.key,
	}

	if item.action == actionCompile {
		temp, err := cache.TempFile(filepath.Base(descriptor.Destination))
		if err != nil {
			return entry, err
		}
		defer os.Remove(temp)

		job := &Job{
			Descriptor: descriptor,
			SourcePath: descriptor.SourcePath,
			OutputPath: temp,
		}

		if err := item.tool.Compile(job); err != nil {
			return entry, err
		}

		if err := cache.StoreObject(item.key, temp); err != nil {
			return entry, err
		}
	}

	if item.action != actionUpToDate {
		if err := CopyFile(cache.ObjectPath(item.key), descriptor.OutputPath); err != nil {
			return entry, err
		}
	}

	hash, err := HashFile(descriptor.OutputPath)
	if err != nil {
		return entry, fmt.Errorf("failed to hash the compiled asset: %w", err)
	}
	entry.OutputHash = hash

	info, err := os.Stat(descriptor.OutputPath)
	if err != nil {
		return entry, fmt.Errorf("failed to inspect the compiled asset: %w", err)
	}
	entry.Size = info.Size()

	return entry, nil
}

// removeOrphans deletes outputs whose descriptor is gone.
//
// Only files the index records as Forge's own are removed. Anything else in
// data/core was put there by hand and is left alone, which is what allows the
// compiled and the hand-placed assets to coexist in one directory.
func removeOrphans(descriptors []*Descriptor, cache *Cache, options Options) (int, error) {
	live := make(map[string]bool, len(descriptors))
	for _, descriptor := range descriptors {
		live[descriptor.VFSPath()] = true
	}

	orphans := make([]string, 0)
	for _, destination := range cache.Destinations() {
		if !live[destination] {
			orphans = append(orphans, destination)
		}
	}
	sort.Strings(orphans)

	removed := 0
	for _, destination := range orphans {
		entry, _ := cache.Lookup(destination)
		path := filepath.Join(options.DataCoreDir, filepath.FromSlash(strings.TrimPrefix(destination, "/")))

		hash, err := HashFile(path)
		if os.IsNotExist(err) {
			// Already gone; just drop the record.
			cache.Forget(destination)
			continue
		} else if err != nil {
			return removed, fmt.Errorf("failed to hash %s: %w", destination, err)
		}

		if hash != entry.OutputHash {
			// The file was changed after Forge wrote it, so it is no longer
			// safe to assume it is ours to delete.
			fmt.Printf("  keeping %s: modified since it was compiled\n", destination)
			cache.Forget(destination)
			continue
		}

		if err := os.Remove(path); err != nil {
			return removed, fmt.Errorf("failed to remove %s: %w", destination, err)
		}
		pruneEmptyDirs(filepath.Dir(path), options.DataCoreDir)

		cache.Forget(destination)
		removed++
		fmt.Printf("  removed %s\n", destination)
	}

	return removed, nil
}

// pruneEmptyDirs removes directories left empty by a deletion, stopping at the
// root. os.Remove only succeeds on empty directories, so this cannot take out
// a directory that still holds hand-placed files.
func pruneEmptyDirs(dir, root string) {
	for dir != root && strings.HasPrefix(dir, root) {
		if err := os.Remove(dir); err != nil {
			return
		}
		dir = filepath.Dir(dir)
	}
}

// cacheKey combines everything that affects the output: the tool, its
// fingerprint, the descriptor's options, the destination and the source's
// contents. Any change to those produces a different artifact.
func cacheKey(descriptor *Descriptor, tool Tool) (string, error) {
	fingerprint, err := tool.Fingerprint(descriptor)
	if err != nil {
		return "", err
	}

	sourceHash, err := HashFile(descriptor.SourcePath)
	if err != nil {
		return "", fmt.Errorf("failed to hash source '%s': %w", descriptor.Source, err)
	}

	var buffer strings.Builder
	buffer.WriteString(tool.Name())
	buffer.WriteByte(0)
	buffer.WriteString(fingerprint)
	buffer.WriteByte(0)
	buffer.Write(descriptor.Options)
	buffer.WriteByte(0)
	buffer.WriteString(descriptor.Destination)
	buffer.WriteByte(0)
	buffer.WriteString(sourceHash)

	sum := xxh3.HashString128(buffer.String())

	return fmt.Sprintf("%016x%016x", sum.Hi, sum.Lo), nil
}

// Clean removes every artifact Forge produced, along with the cache itself.
func Clean(options Options) (int, error) {
	cache, err := OpenCache(options.CacheDir)
	if err != nil {
		return 0, err
	}

	removed, err := removeOrphans(nil, cache, options)
	if err != nil {
		return removed, err
	}

	if err := os.RemoveAll(options.CacheDir); err != nil {
		return removed, fmt.Errorf("failed to remove the cache: %w", err)
	}

	return removed, nil
}
