#!/bin/bash
set -euo pipefail

# Installs the emsdk into pandora/ext
EXT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/ext"
EMSDK_VERSION=4.0.17

mkdir -p "$EXT_DIR"
if [ ! -d "$EXT_DIR/emsdk" ]; then
    git clone https://github.com/emscripten-core/emsdk.git "$EXT_DIR/emsdk"
fi

cd "$EXT_DIR/emsdk"
./emsdk install "$EMSDK_VERSION"
./emsdk activate "$EMSDK_VERSION"
