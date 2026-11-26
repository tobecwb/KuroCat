#!/usr/bin/env bash
# macOS pixel regression against the upstream renderer before the eyelid fix; no app is launched.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CHECK_DIR="$(mktemp -d)"
trap 'rm -rf "$CHECK_DIR"' EXIT
python3 - "$ROOT" "$CHECK_DIR" <<'PY'
from pathlib import Path
import subprocess, sys
root, out = map(Path, sys.argv[1:])
physics = (root/'Sources/CatPhysics.swift').read_text()
a = physics.index('        let petClose', physics.index('var effectiveEyeClosure'))
b = physics.index('\n    }', a)
(out/'CatPhysics.swift').write_text(physics[:a]+'        return testEyeClosure'+physics[b:])
original = subprocess.check_output(['git','-C',str(root),'show','073bc6d97c76a99db72a0eb103a2bd3245fe1ab1:Sources/CatRenderer.swift'], text=True)
for name, source in [('OriginalRenderer',original),('FixedRenderer',(root/'Sources/CatRenderer.swift').read_text())]:
    source = source.replace('enum CatRenderer {','enum '+name+' {').replace('private static func drawEye3D','static func drawEye3D')
    if name == 'OriginalRenderer': source = source.replace('CatHitZone','OriginalCatHitZone')
    (out/(name+'.swift')).write_text(source)
PY
swiftc -module-cache-path "$CHECK_DIR/cache" \
    "$ROOT/Sources/KeyMap.swift" "$ROOT/Sources/InputEngine.swift" \
    "$ROOT/Sources/CoatPatterns.swift" "$ROOT/Sources/BeagleAtlas.swift" "$CHECK_DIR/CatPhysics.swift" "$CHECK_DIR/OriginalRenderer.swift" "$CHECK_DIR/FixedRenderer.swift" \
    "$ROOT/tests/eye-closure/main.swift" -o "$CHECK_DIR/check" \
    -framework AppKit -framework CoreGraphics -framework QuartzCore -framework IOKit -framework ApplicationServices -framework ImageIO
"$CHECK_DIR/check"
