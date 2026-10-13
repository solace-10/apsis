package cmd

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"

	"wings_of_steel/forge/server"
)

var servePort int

var ServeCmd = &cobra.Command{
	Use:   "serve",
	Short: "Run local HTTP server",
	Long: `Start a local HTTP server to serve game assets for development.

The server includes CORS headers for local development and serves
files from game/bin/ on the specified port (default 8080).

The game's web build expects assets at http://127.0.0.1:8080/`,
	Run: func(cmd *cobra.Command, args []string) {
		gameBinDir, err := server.FindGameBinDir()
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}

		srv := server.New(gameBinDir, servePort)
		if err := srv.Start(); err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}
	},
}

func init() {
	ServeCmd.Flags().IntVarP(&servePort, "port", "p", 8080, "Port to serve on")
}
