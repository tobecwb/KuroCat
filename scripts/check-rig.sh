#!/usr/bin/env bash
# Part-space mapping parity: every port must map skin points exactly like tests/rig/reference.py (from shared/rig.json).
#  1. generated rig blocks are current (scripts/gen-rig.py --check) and golden.tsv matches rig.json
#  2. C++ (shared/Rig.hpp, used by Windows + Linux)
#  3. web (homepage/pet/renderer.js, plus KuroCat-Workshop/renderer.js when that checkout is next to this one
#     or given in KUROCAT_WORKSHOP)
#  4. Swift (Sources/CatRenderer.swift), only where swiftc exists (macOS / CI)
#  5. web skin rules (spec.js: spaces, shape ranges, limits, formatVersion) vs shared/SkinSpec.hpp + CatPhysics.swift
#     and homepage/pet/spec.js is an exact copy of the workshop's (scripts/sync-web-spec.sh)
#  6. homepage/pet/renderer.js is an exact copy of KuroCat-Workshop/renderer.js (scripts/sync-web-renderer.sh)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
WORKSHOP="${KUROCAT_WORKSHOP:-$ROOT/../KuroCat-Workshop}"

if [ -f "$WORKSHOP/renderer.js" ]; then
    python3 "$ROOT/scripts/gen-rig.py" --check --workshop "$WORKSHOP"
else
    python3 "$ROOT/scripts/gen-rig.py" --check
fi
python3 "$ROOT/tests/rig/reference.py" --check
bash "$ROOT/scripts/sync-web-spec.sh" --check
bash "$ROOT/scripts/sync-web-renderer.sh" --check

CXX="${CXX:-c++}"
"$CXX" -std=c++17 -O2 "$ROOT/tests/rig/rig_test.cpp" -o "$WORK/rig_test"
"$WORK/rig_test" "$ROOT/tests/rig/golden.tsv"

if command -v node > /dev/null; then
    if [ -f "$WORKSHOP/renderer.js" ]; then
        node "$ROOT/tests/rig/rig_test.mjs" "$WORKSHOP"
    else
        node "$ROOT/tests/rig/rig_test.mjs"
    fi
    node "$ROOT/tests/rig/rules_test.mjs"   # web skin rules vs the desktop parsers
else
    echo "skip: node not found (web renderers not checked)"
fi

if command -v swiftc > /dev/null; then
    python3 - "$ROOT" "$WORK" <<'PY'
from pathlib import Path
import sys
root, out = map(Path, sys.argv[1:])
(out / 'CatPhysics.swift').write_text((root / 'Sources/CatPhysics.swift').read_text().replace('private(set) ', ''))
src = (root / 'Sources/CatRenderer.swift').read_text()
src = src.replace('enum CatRenderer {', 'enum CurrentRenderer {').replace('CatSettings.shared', 'RenderSettings.shared')
for fn in ('headPoint', 'bodyPoint'):
    src = src.replace(f'private static func {fn}(', f'static func {fn}(')
(out / 'CurrentRenderer.swift').write_text(src)
PY
    swiftc -module-cache-path "$WORK/cache" \
        "$ROOT/Sources/KeyMap.swift" "$ROOT/Sources/InputEngine.swift" "$ROOT/Sources/CoatPatterns.swift" "$ROOT/Sources/BeagleAtlas.swift" \
        "$WORK/CatPhysics.swift" "$WORK/CurrentRenderer.swift" "$ROOT/tests/rig/main.swift" -o "$WORK/rig_swift" \
        -framework AppKit -framework CoreGraphics -framework QuartzCore -framework IOKit -framework ApplicationServices
    "$WORK/rig_swift" "$ROOT/tests/rig/golden.tsv"
else
    echo "skip: swiftc not found (macOS renderer is checked in CI)"
fi
