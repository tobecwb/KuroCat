#!/usr/bin/env bash
# Native renderer regression and action sheets; never launches the app or changes settings.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CHECK_DIR="$(mktemp -d)"
trap 'rm -rf "$CHECK_DIR"' EXIT
python3 "$ROOT/scripts/generate-coat-patterns.py" --check
python3 - "$ROOT" "$CHECK_DIR" <<'PY'
from pathlib import Path
import subprocess,sys
root,out=map(Path,sys.argv[1:])
p=(root/'Sources/CatPhysics.swift').read_text().replace('private(set) ','')
(out/'CatPhysics.swift').write_text(p)
baseline='e30e0c15209939d1aee4f2692c8e9e422050fb1b'
sources=[('LegacyRenderer',subprocess.check_output(['git','-C',str(root),'show',baseline+':Sources/CatRenderer.swift'],text=True)),('CurrentRenderer',(root/'Sources/CatRenderer.swift').read_text())]
patterns=subprocess.check_output(['git','-C',str(root),'show',baseline+':Sources/CoatPatterns.swift'],text=True)
(out/'BaselinePatterns.swift').write_text(patterns.replace('CoatPatterns','BaselinePatterns').replace('CoatMark','BaselineMark'))
for name,source in sources:
 source=source.replace('enum CatRenderer {','enum '+name+' {').replace('CatSettings.shared','RenderSettings.shared')
 source=source.replace('private static func coatMarkPoint','static func coatMarkPoint').replace('private static func projectHead3D','static func projectHead3D')
 if name!='CurrentRenderer':source=source.replace('CatHitZone',name+'CatHitZone')
 (out/(name+'.swift')).write_text(source)
PY
swiftc -module-cache-path "$CHECK_DIR/cache" \
 "$CHECK_DIR/BaselinePatterns.swift" "$ROOT/Sources/KeyMap.swift" "$ROOT/Sources/InputEngine.swift" "$ROOT/Sources/CoatPatterns.swift" \
 "$CHECK_DIR/CatPhysics.swift" "$CHECK_DIR/LegacyRenderer.swift" "$CHECK_DIR/CurrentRenderer.swift" \
 "$ROOT/tests/skins/main.swift" -o "$CHECK_DIR/check" \
 -framework AppKit -framework CoreGraphics -framework QuartzCore -framework IOKit -framework ApplicationServices
# Expected skin error texts are the Chinese ones.
KUROCAT_LANGUAGE=zh KUROCAT_ROOT="$ROOT" "$CHECK_DIR/check" "${1:-$CHECK_DIR}"
