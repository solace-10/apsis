package cmd

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"

	"wings_of_steel/forge/upload"
)

var uploadForce bool

var UploadCmd = &cobra.Command{
	Use:   "upload",
	Short: "Upload assets to Cloudflare R2",
	Long: `Upload game assets to Cloudflare R2 via the Forge worker.

Reads the manifest from game/bin/manifest.json
and uploads each file to the Cloudflare Worker endpoint.

By default, checks if files already exist and skips them.
Use --force to upload all files regardless.

Run 'forge manifest' first to generate the manifest.`,
	Run: func(cmd *cobra.Command, args []string) {
		manifestPath, assetsDir, err := upload.FindPaths()
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}

		// Check manifest exists
		if _, err := os.Stat(manifestPath); os.IsNotExist(err) {
			fmt.Fprintf(os.Stderr, "Error: manifest not found at %s\n", manifestPath)
			fmt.Fprintf(os.Stderr, "Run 'forge manifest' first to generate it.\n")
			os.Exit(1)
		}

		fmt.Printf("Manifest: %s\n", manifestPath)
		fmt.Printf("Assets:   %s\n\n", assetsDir)

		uploader, err := upload.NewUploader(manifestPath, assetsDir)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}

		if err := uploader.Upload(uploadForce); err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}
	},
}

func init() {
	UploadCmd.Flags().BoolVarP(&uploadForce, "force", "f", false, "Upload all files, skip existence check")
}
