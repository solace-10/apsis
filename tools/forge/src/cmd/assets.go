package cmd

import (
	"fmt"
	"os"
	"runtime"

	"github.com/spf13/cobra"

	"wings_of_steel/forge/assets"
	"wings_of_steel/forge/paths"
)

var (
	assetsForce   bool
	assetsJobs    int
	assetsClean   bool
	assetsVerbose bool
)

var AssetsCmd = &cobra.Command{
	Use:   "assets",
	Short: "Compile assets into the game's data directory",
	Long: `Compile game assets described by descriptors into their shipping form.

Every '*.asset.json' file under game/assets is read as a descriptor naming a
source file, an optional destination, and any format-specific options. The
tool used is chosen from the source's extension: images are encoded to KTX2,
and anything without a dedicated tool is copied verbatim.

Compiled artifacts are cached by content hash in .forge-cache, so unchanged
assets are not compiled twice.

Source: game/assets/
Output: game/bin/data/core/

Run this before 'forge manifest' so the manifest describes compiled output.`,
	Run: func(cmd *cobra.Command, args []string) {
		assetsDir, err := paths.AssetsDir()
		if err != nil {
			fail(err)
		}

		dataCoreDir, err := paths.DataCoreDir()
		if err != nil {
			fail(err)
		}

		cacheDir, err := paths.CacheDir()
		if err != nil {
			fail(err)
		}

		options := assets.Options{
			AssetsDir:   assetsDir,
			DataCoreDir: dataCoreDir,
			CacheDir:    cacheDir,
			Force:       assetsForce,
			Jobs:        assetsJobs,
			Verbose:     assetsVerbose,
		}

		if assetsClean {
			fmt.Printf("Cleaning compiled assets in %s\n", dataCoreDir)
			removed, err := assets.Clean(options)
			if err != nil {
				fail(err)
			}
			fmt.Printf("\nRemoved %d compiled assets and the cache.\n", removed)
			return
		}

		if _, err := os.Stat(assetsDir); os.IsNotExist(err) {
			fmt.Printf("No assets directory at %s, nothing to compile.\n", assetsDir)
			return
		}

		fmt.Printf("Compiling assets...\n")
		fmt.Printf("Source: %s\n", assetsDir)
		fmt.Printf("Output: %s\n\n", dataCoreDir)

		result, err := assets.Compile(options)
		if err != nil {
			fail(err)
		}

		fmt.Printf("\n%d compiled, %d restored from cache, %d up to date, %d removed.\n",
			result.Compiled, result.Restored, result.UpToDate, result.Removed)
	},
}

func fail(err error) {
	fmt.Fprintf(os.Stderr, "Error: %v\n", err)
	os.Exit(1)
}

func init() {
	AssetsCmd.Flags().BoolVarP(&assetsForce, "force", "f", false, "Recompile every asset, ignoring the cache")
	AssetsCmd.Flags().IntVarP(&assetsJobs, "jobs", "j", runtime.NumCPU(), "Number of assets to compile concurrently")
	AssetsCmd.Flags().BoolVar(&assetsClean, "clean", false, "Remove compiled assets and the cache")
	AssetsCmd.Flags().BoolVarP(&assetsVerbose, "verbose", "v", false, "Also report assets that are already up to date")
}
