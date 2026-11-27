#!/usr/bin/env bash
# The skin rules for the web live in KuroCat-Workshop/spec.js; the kurocat.cc pet uses an exact copy.
#   bash scripts/sync-web-spec.sh            copy the workshop's spec.js over homepage/pet/spec.js, rebuild the pages
#   bash scripts/sync-web-spec.sh --check    exit 1 if the copy differs (scripts/check-rig.sh runs this)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORKSHOP="${KUROCAT_WORKSHOP:-$ROOT/../KuroCat-Workshop}"
SRC="$WORKSHOP/spec.js"
DST="$ROOT/homepage/pet/spec.js"
if [ ! -f "$SRC" ]; then
    echo "skip: $SRC not found (set KUROCAT_WORKSHOP to the KuroCat-Workshop checkout)"
    exit 0
fi
if [ "${1:-}" = "--check" ]; then
    if ! cmp -s "$SRC" "$DST"; then
        echo "homepage/pet/spec.js differs from KuroCat-Workshop/spec.js: run bash scripts/sync-web-spec.sh" >&2
        exit 1
    fi
    exit 0
fi
cp "$SRC" "$DST"
python3 "$ROOT/scripts/homepage/build.py"
echo "synced $DST"
