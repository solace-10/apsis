#!/usr/bin/env bash
# Sync the pandora/ subtree with the shared engine repository.
#
#   ./scripts/pandora.sh status   # what is unpushed here / not yet pulled from upstream
#   ./scripts/pandora.sh pull     # merge upstream pandora main into pandora/ (squashed)
#   ./scripts/pandora.sh push     # push pandora/ commits upstream (main branch of this repo only)
#   ./scripts/pandora.sh sync     # pull, then push
#
# pandora/ is a plain directory in this repo: commit engine and game changes together as usual,
# and sync when they land on main. Other projects pick the changes up with their own `pull`.
#
# PANDORA_REMOTE overrides the upstream URL (e.g. a local bare clone, for testing).
set -euo pipefail

REMOTE="${PANDORA_REMOTE:-git@github.com:solace-10/pandora.git}"
BRANCH=main
PREFIX=pandora

cd "$(git rev-parse --show-toplevel)"

log() { echo "[pandora] $*"; }
die() {
    echo "[pandora] $*" >&2
    exit 1
}

require_clean() {
    git diff --quiet && git diff --cached --quiet ||
        die "working tree has uncommitted changes; commit or stash first"
}

require_main() {
    local branch
    branch="$(git symbolic-ref -q --short HEAD || true)"
    [ "$branch" = "main" ] || [ "${PANDORA_ALLOW_BRANCH:-}" = 1 ] ||
        die "on '${branch:-detached HEAD}', not main: push engine changes after they land on main" \
            "(set PANDORA_ALLOW_BRANCH=1 to override)"
}

# Synthesises pandora's history from this repo's commits. Deterministic, so commits that were
# already pushed come out with the same SHAs as upstream.
split() {
    git subtree split --prefix="$PREFIX" HEAD 2>/dev/null
}

fetch_upstream() {
    git fetch -q "$REMOTE" "$BRANCH"
    git rev-parse FETCH_HEAD
}

cmd_status() {
    local upstream local_split
    upstream="$(fetch_upstream)"
    local_split="$(split)"

    local unpushed unpulled
    unpushed="$(git log --oneline "$upstream..$local_split")"
    unpulled="$(git log --oneline "$local_split..$upstream")"

    if [ -z "$unpushed" ] && [ -z "$unpulled" ]; then
        log "in sync with upstream $BRANCH (${upstream:0:7})"
        return
    fi
    if [ -n "$unpushed" ]; then
        log "not yet pushed upstream:"
        echo "$unpushed" | sed 's/^/    /'
    fi
    if [ -n "$unpulled" ]; then
        log "upstream changes not yet pulled:"
        echo "$unpulled" | sed 's/^/    /'
    fi
}

cmd_pull() {
    require_clean
    log "pulling $REMOTE $BRANCH"
    git subtree pull --prefix="$PREFIX" --squash "$REMOTE" "$BRANCH" \
        -m "Pandora pull" ||
        die "pull stopped (conflict?): resolve, commit, then re-run"
}

cmd_push() {
    require_clean
    require_main
    log "pushing $PREFIX/ to $REMOTE $BRANCH"
    git subtree push --prefix="$PREFIX" "$REMOTE" "$BRANCH" ||
        die "push rejected: upstream has moved on, run '$0 sync'"
}

case "${1:-}" in
status) cmd_status ;;
pull) cmd_pull ;;
push) cmd_push ;;
sync)
    cmd_pull
    cmd_push
    ;;
*)
    sed -n '2,13p' "$0" | sed 's/^# \{0,1\}//'
    exit 1
    ;;
esac
