package cmd

import (
	"fmt"

	"github.com/spf13/cobra"
)

var PackageCmd = &cobra.Command{
	Use:   "package",
	Short: "Generate manifest, upload, and inject in one step",
	Long: `Run the complete asset packaging pipeline:

1. Generate manifest from game assets
2. Upload assets to Cloudflare R2
3. Inject manifest into game.html

This is the recommended command for CI/CD pipelines.`,
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println("Running full package pipeline...")
		// TODO: Implement full pipeline
		fmt.Println("TODO: Run manifest, upload, inject in sequence")
	},
}
