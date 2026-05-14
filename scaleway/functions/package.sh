#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

OUTPUT="Function.zip"

rm -f "$OUTPUT"
zip -r "$OUTPUT" Cargo.lock Cargo.toml src

echo "Created $OUTPUT"
