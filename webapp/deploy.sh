#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
REMOTE_USER="cpe0004521"
REMOTE_HOST="51.159.140.42"
REMOTE_PATH="/home/cpe0004521/apsis.earth"

if [[ ! -d "${BUILD_DIR}" ]]; then
    echo "Error: build directory not found at ${BUILD_DIR}" >&2
    echo "Run 'npm run build' first." >&2
    exit 1
fi

echo "Syncing ${BUILD_DIR}/ to ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PATH}"

tar -C "${BUILD_DIR}" -cf - . | ssh "${REMOTE_USER}@${REMOTE_HOST}" "
    set -e
    mkdir -p '${REMOTE_PATH}'
    find '${REMOTE_PATH}' -mindepth 1 -delete
    tar -C '${REMOTE_PATH}' -xf -
"

echo "Deploy complete."
