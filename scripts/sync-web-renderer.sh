#!/usr/bin/env bash
# The web cat renderer lives in KuroCat-Workshop/renderer.js; the kurocat.cc pet uses an exact copy.
# Make renderer changes in the workshop, then copy them over with this script (same idea as sync-web-spec.sh).
#   bash scripts/sync-web-renderer.sh            copy the workshop's renderer.js over homepage/pet/renderer.js, rebuild the pages
#   bash scripts/sync-web-renderer.sh --check    exit 1 if the copy differs (scripts/check-rig.sh runs this)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORKSHOP="${KUROCAT_WORKSHOP:-$ROOT/../KuroCat-Workshop}"
SRC="$WORKSHOP/renderer.js"
DST="$ROOT/homepage/pet/renderer.js"
if [ ! -f "$SRC" ]; then
    echo "skip: $SRC not found (set KUROCAT_WORKSHOP to the KuroCat-Workshop checkout)"
    exit 0
fi
if [ "${1:-}" = "--check" ]; then
    if ! cmp -s "$SRC" "$DST"; then
        echo "homepage/pet/renderer.js differs from KuroCat-Workshop/renderer.js: edit the workshop copy, then run bash scripts/sync-web-renderer.sh" >&2
        exit 1
    fi
    exit 0
fi
cp "$SRC" "$DST"
python3 "$ROOT/scripts/homepage/build.py"
echo "synced $DST"
