package cmd

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"

	"wings_of_steel/forge/manifest"
	"wings_of_steel/forge/paths"
)

var ManifestCmd = &cobra.Command{
	Use:   "manifest",
	Short: "Generate asset manifest",
	Long: `Generate a manifest.json file from the game assets.

The manifest contains the path, XXHash3-64 hash, and size of each asset file.
This manifest is used by the web build to download and verify assets.

Source: game/bin/data/core/
Output: game/bin/manifest.json`,
	Run: func(cmd *cobra.Command, args []string) {
		sourceDir, err := paths.DataCoreDir()
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}

		outputDir, err := paths.GameBinDir()
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}

		fmt.Printf("Generating manifest...\n")
		fmt.Printf("Source: %s\n", sourceDir)
		fmt.Printf("Output: %s\n\n", outputDir)

		gen := manifest.NewGenerator(sourceDir, outputDir)
		if err := gen.Generate(); err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}
	},
}
