#!/usr/bin/env bash
# Idempotent per-worktree setup for Apsis.
#
# Run from anywhere inside a checkout (fresh worktrees especially):
#   ./scripts/worktree-setup.sh
#
# Creates only what's missing, borrowing gitignored prerequisites from the
# canonical checkout at ~/dev/apsis. Safe to re-run at any time; also
# re-attaches pandora's HEAD if a submodule update left it detached.
#
# Kept deliberately in step with the sibling danus_garden script of the same
# name — both repos share the pandora engine and the same worktree layout, so
# the setup behaviour should not drift between them.
set -euo pipefail

MAIN_CHECKOUT="${APSIS_MAIN_CHECKOUT:-$HOME/dev/apsis}"

cd "$(git rev-parse --show-toplevel)"

log() { echo "[worktree-setup] $*"; }

# --- 1. pandora submodule -----------------------------------------------------
if [ ! -e pandora/CMakeLists.txt ]; then
    log "initializing pandora submodule"
    git submodule update --init pandora
fi

# Attach pandora's HEAD so commits made in it aren't dangling. `submodule
# update` always leaves a detached HEAD; point main at it (a no-op or
# fast-forward in the normal case) and track origin/main.
if ! git -C pandora symbolic-ref -q HEAD >/dev/null; then
    log "attaching pandora HEAD to main"
    git -C pandora checkout -B main
    git -C pandora branch --set-upstream-to=origin/main main 2>/dev/null || true
fi

# --- 2. pandora/ext (emsdk + FetchContent deps, ~4 GB, gitignored) ------------
# Shared with the canonical checkout rather than re-downloaded. Note the
# consequence: a dependency bump or setup_emscripten.sh run in either tree is
# seen by both.
if [ ! -e pandora/ext ]; then
    if [ -e "$MAIN_CHECKOUT/pandora/ext" ]; then
        log "symlinking pandora/ext -> $MAIN_CHECKOUT/pandora/ext"
        ln -s "$MAIN_CHECKOUT/pandora/ext" pandora/ext
    else
        log "WARNING: $MAIN_CHECKOUT/pandora/ext not found; run pandora/scripts/setup_emscripten.sh"
    fi
fi

# --- 3. .env (FORGE_AUTH_KEY_SECRET, gitignored) ------------------------------
if [ ! -f .env ]; then
    if [ -f "$MAIN_CHECKOUT/.env" ]; then
        log "copying .env from $MAIN_CHECKOUT"
        cp "$MAIN_CHECKOUT/.env" .env
    else
        log "WARNING: $MAIN_CHECKOUT/.env not found; forge upload will fail"
    fi
fi

# --- 4. game/bin/manifest.json (gitignored, generated) ------------------------
# Unlike danus_garden, Apsis's `add_custom_command` in game/CMakeLists.txt
# declares OUTPUT "${MANIFEST_OUTPUT}", so CMake can generate this itself and a
# fresh worktree does not hard-fail without it. Bootstrapping anyway keeps the
# two repos' setup behaviour identical and costs one cheap forge invocation.
if [ ! -f game/bin/manifest.json ]; then
    log "bootstrapping game/bin/manifest.json"
    ./pandora/tools/forge/bin/forge manifest
fi

# --- 5. git config guardrails (shared across all worktrees) -------------------
# on-demand: `git push` in this repo pushes pandora's commits first, so the
# outer pointer can never reference a SHA that isn't upstream.
git config push.recurseSubmodules on-demand
# Keep pandora in sync on checkout/pull without a manual `submodule update`.
git config submodule.recurse true

log "done"
