package assets

import "fmt"

func init() {
	// Registered without extensions: it is the fallback for anything no other
	// tool claims, which ToolFor resolves by name.
	Register(&copyTool{})
}

// copyTool passes a source file through unchanged. It is what any extension
// without a dedicated tool falls back to.
type copyTool struct{}

func (t *copyTool) Name() string {
	return FallbackToolName
}

func (t *copyTool) OutputExtension(srcExt string) string {
	return srcExt
}

func (t *copyTool) Fingerprint(d *Descriptor) (string, error) {
	// The copy tool takes no options, so any that were given are a mistake
	// worth reporting rather than ignoring.
	var options struct{}
	if err := d.DecodeOptions(&options); err != nil {
		return "", fmt.Errorf("the 'copy' tool takes no options: %w", err)
	}

	return "copy/1", nil
}

func (t *copyTool) Compile(job *Job) error {
	return CopyFile(job.SourcePath, job.OutputPath)
}
