package main

import (
	"os"

	"github.com/spf13/cobra"

	"wings_of_steel/forge/cmd"
)

var rootCmd = &cobra.Command{
	Use:   "forge",
	Short: "Forge - Web asset pipeline tool for Pandora",
	Long: `Forge is a command-line tool for managing Pandora game assets for web deployments.

It provides commands for:
  - Compiling assets into their shipping form
  - Generating asset manifests
  - Uploading assets to Cloudflare R2
  - Serving assets locally for development
  - Packaging everything for deployment`,
}

func init() {
	rootCmd.AddCommand(cmd.AssetsCmd)
	rootCmd.AddCommand(cmd.ManifestCmd)
	rootCmd.AddCommand(cmd.UploadCmd)
	rootCmd.AddCommand(cmd.ServeCmd)
	rootCmd.AddCommand(cmd.PackageCmd)
	rootCmd.AddCommand(cmd.InjectCmd)
}

func main() {
	if err := rootCmd.Execute(); err != nil {
		os.Exit(1)
	}
}
