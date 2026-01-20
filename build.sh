#!/bin/bash

# Validate arguments
if [ $# -ne 2 ]; then
    echo "Usage: $0 <platform> <build_type>"
    echo "  platform: windows, linux, web"
    echo "  build_type: debug, release"
    exit 1
fi

PLATFORM="$1"
BUILD_TYPE="$2"

# Validate platform
if [[ ! "$PLATFORM" =~ ^(windows|linux|web)$ ]]; then
    echo "Error: Invalid platform '$PLATFORM'. Must be: windows, linux, web"
    exit 1
fi

# Validate build type
if [[ ! "$BUILD_TYPE" =~ ^(debug|release)$ ]]; then
    echo "Error: Invalid build type '$BUILD_TYPE'. Must be: debug, release"
    exit 1
fi

PRESET="${BUILD_TYPE}-${PLATFORM}"

echo "Configuring with preset: $PRESET"
if ! cmake --preset "$PRESET"; then
    echo "Error: Configuration failed"
    exit 1
fi

echo "Building with preset: $PRESET"
cmake --build --preset "$PRESET"
