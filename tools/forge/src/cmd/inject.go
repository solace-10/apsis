package cmd

import (
	"fmt"

	"github.com/spf13/cobra"
)

var injectInput string
var injectOutput string
var injectManifest string

var InjectCmd = &cobra.Command{
	Use:   "inject",
	Short: "Inject manifest into game.html",
	Long: `Inject the asset manifest into the Emscripten-generated game.html file.

This embeds the manifest data directly into the HTML, allowing the game
to load without first fetching a separate manifest.json file.`,
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println("Injecting manifest into HTML...")
		fmt.Printf("Input:    %s\n", injectInput)
		fmt.Printf("Output:   %s\n", injectOutput)
		fmt.Printf("Manifest: %s\n", injectManifest)
		// TODO: Implement HTML injection
		fmt.Println("TODO: Read HTML, inject manifest, write output")
	},
}

func init() {
	InjectCmd.Flags().StringVarP(&injectInput, "input", "i", "build/debug-web/game/game.html", "Input HTML file")
	InjectCmd.Flags().StringVarP(&injectOutput, "output", "o", "", "Output HTML file (default: same as input)")
	InjectCmd.Flags().StringVarP(&injectManifest, "manifest", "m", "game/bin/data/core/manifest.json", "Manifest file to inject")
}
