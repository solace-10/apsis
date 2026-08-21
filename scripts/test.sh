#!/bin/bash
#
# Build and run Orbis's test suite.
#
# Usage: ./scripts/test.sh [ctest arguments...]
#   ./scripts/test.sh                       # everything
#   ./scripts/test.sh -R earth_frame        # only tests whose name matches "earth_frame"
#   ./scripts/test.sh --rerun-failed
#
# The tests are native and always built Debug: PANDORA_ASSERT compiles out under NDEBUG, so a
# release suite would quietly stop checking a large number of the invariants it looks like it checks.
#
# Only the `tests` target is built, not the game. The suite links game_lib and therefore pandora, so
# a clean worktree builds the engine once before the first run - see game/tests/CMakeLists.txt for
# why that trade was made.

set -euo pipefail

cd "$(dirname "$0")/.."

PRESET="debug-linux"
BUILD_DIR="build/${PRESET}"

echo "==> Configuring ($PRESET)"
cmake --preset "$PRESET" > /dev/null

echo "==> Building tests"
cmake --build "$BUILD_DIR" --target tests

echo "==> Running tests"
ctest --test-dir "$BUILD_DIR" --output-on-failure "$@"
