package upload

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"time"

	"github.com/schollz/progressbar/v3"

	"wings_of_steel/forge/paths"
)

const WorkerURL = "https://forge-cloudflare-worker.wings-of-steel.workers.dev"

type ManifestEntry struct {
	Path string `json:"path"`
	Hash string `json:"hash"`
	Size int64  `json:"size"`
}

type Uploader struct {
	manifestPath string
	assetsDir    string
	authKey      string
	client       *http.Client
}

func NewUploader(manifestPath, assetsDir string) (*Uploader, error) {
	authKey := os.Getenv("FORGE_AUTH_KEY_SECRET")
	if authKey == "" {
		return nil, fmt.Errorf("FORGE_AUTH_KEY_SECRET environment variable is not set")
	}

	return &Uploader{
		manifestPath: manifestPath,
		assetsDir:    assetsDir,
		authKey:      authKey,
		client: &http.Client{
			Timeout: 5 * time.Minute,
		},
	}, nil
}

func (u *Uploader) Upload(force bool) error {
	// Read manifest
	manifestData, err := os.ReadFile(u.manifestPath)
	if err != nil {
		return fmt.Errorf("failed to read manifest: %w", err)
	}

	var entries []ManifestEntry
	if err := json.Unmarshal(manifestData, &entries); err != nil {
		return fmt.Errorf("failed to parse manifest: %w", err)
	}

	var toUpload []ManifestEntry
	var uploadSize int64
	var skippedCount int

	if force {
		// Force mode: upload everything
		fmt.Printf("Force mode: uploading all %d files\n\n", len(entries))
		toUpload = entries
		for _, entry := range entries {
			uploadSize += entry.Size
		}
	} else {
		// Check which files need uploading
		fmt.Printf("Checking %d files...\n", len(entries))

		checkBar := progressbar.NewOptions(
			len(entries),
			progressbar.OptionSetDescription("Checking"),
			progressbar.OptionSetWidth(40),
			progressbar.OptionShowCount(),
			progressbar.OptionSetTheme(progressbar.Theme{
				Saucer:        "=",
				SaucerHead:    ">",
				SaucerPadding: " ",
				BarStart:      "[",
				BarEnd:        "]",
			}),
		)

		for _, entry := range entries {
			exists, err := u.fileExists(entry.Hash)
			if err != nil {
				fmt.Printf("\nWarning: failed to check %s: %v\n", entry.Path, err)
				// Assume it needs uploading if check fails
				toUpload = append(toUpload, entry)
				uploadSize += entry.Size
			} else if !exists {
				toUpload = append(toUpload, entry)
				uploadSize += entry.Size
			}
			checkBar.Add(1)
		}

		skippedCount = len(entries) - len(toUpload)
		fmt.Printf("\n\n%d files already exist, %d need uploading (%.2f MB)\n\n",
			skippedCount, len(toUpload), float64(uploadSize)/(1024*1024))

		if len(toUpload) == 0 {
			fmt.Println("Nothing to upload.")
			return nil
		}
	}

	// Create progress bar for uploads
	bar := progressbar.NewOptions64(
		uploadSize,
		progressbar.OptionSetDescription("Uploading"),
		progressbar.OptionSetWidth(40),
		progressbar.OptionShowBytes(true),
		progressbar.OptionShowCount(),
		progressbar.OptionSetTheme(progressbar.Theme{
			Saucer:        "=",
			SaucerHead:    ">",
			SaucerPadding: " ",
			BarStart:      "[",
			BarEnd:        "]",
		}),
	)

	// Upload each file
	var uploadedCount int
	var failedCount int

	for _, entry := range toUpload {
		filePath := filepath.Join(u.assetsDir, filepath.FromSlash(entry.Path))

		err := u.uploadFile(filePath, entry.Hash)
		if err != nil {
			fmt.Printf("\nFailed to upload %s: %v\n", entry.Path, err)
			failedCount++
		} else {
			uploadedCount++
		}

		bar.Add64(entry.Size)
	}

	fmt.Printf("\n\nUpload complete: %d uploaded, %d failed, %d skipped\n",
		uploadedCount, failedCount, skippedCount)

	if failedCount > 0 {
		return fmt.Errorf("%d files failed to upload", failedCount)
	}

	return nil
}

func (u *Uploader) fileExists(hash string) (bool, error) {
	url := fmt.Sprintf("%s/%s", WorkerURL, hash)
	req, err := http.NewRequest(http.MethodHead, url, nil)
	if err != nil {
		return false, fmt.Errorf("failed to create request: %w", err)
	}

	resp, err := u.client.Do(req)
	if err != nil {
		return false, fmt.Errorf("request failed: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode == 200 {
		return true, nil
	} else if resp.StatusCode == 404 {
		return false, nil
	}

	return false, fmt.Errorf("unexpected status: %d", resp.StatusCode)
}

func (u *Uploader) uploadFile(filePath, hash string) error {
	// Read file
	data, err := os.ReadFile(filePath)
	if err != nil {
		return fmt.Errorf("failed to read file: %w", err)
	}

	// Create PUT request
	url := fmt.Sprintf("%s/%s", WorkerURL, hash)
	req, err := http.NewRequest(http.MethodPut, url, bytes.NewReader(data))
	if err != nil {
		return fmt.Errorf("failed to create request: %w", err)
	}

	req.Header.Set("Content-Type", "application/octet-stream")
	req.Header.Set("X-Custom-Auth-Key", u.authKey)

	// Send request
	resp, err := u.client.Do(req)
	if err != nil {
		return fmt.Errorf("request failed: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		body, _ := io.ReadAll(resp.Body)
		return fmt.Errorf("server returned %d: %s", resp.StatusCode, string(body))
	}

	return nil
}

func FindPaths() (manifestPath, assetsDir string, err error) {
	manifestPath, err = paths.ManifestPath()
	if err != nil {
		return "", "", err
	}

	assetsDir, err = paths.DataCoreDir()
	if err != nil {
		return "", "", err
	}

	return manifestPath, assetsDir, nil
}
