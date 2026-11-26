#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
APP_NAME="KuroCat"
APP_BUNDLE="$ROOT/${APP_NAME}.app"
CONTENTS="$APP_BUNDLE/Contents"
MACOS_DIR="$CONTENTS/MacOS"
RES_DIR="$CONTENTS/Resources"

echo "==> Building ${APP_NAME}.app..."
mkdir -p "$MACOS_DIR" "$RES_DIR"

cp "$ROOT/Resources/Info.plist" "$CONTENTS/Info.plist"

if [ -f "$ROOT/Resources/AppIcon.icns" ]; then
    cp "$ROOT/Resources/AppIcon.icns" "$RES_DIR/AppIcon.icns"
else
# Generate cute black cat AppIcon.icns using Python + iconutil
python3 - <<'PYEOF' "$RES_DIR"
import os, subprocess, sys
from PIL import Image, ImageDraw

res_dir = sys.argv[1]
iconset = os.path.join(res_dir, "AppIcon.iconset")
os.makedirs(iconset, exist_ok=True)

def make_icon(size):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    s = size / 512.0

    # Rounded macOS squircle background (warm soft cream)
    pad = int(28 * s)
    rad = int(112 * s)
    d.rounded_rectangle([pad, pad, size - pad, size - pad], radius=rad, fill=(244, 242, 234, 255))

    # Matte black cat ears
    black = (22, 22, 26, 255)
    d.polygon([
        (118*s, 238*s), (132*s, 96*s), (222*s, 178*s)
    ], fill=black)
    d.polygon([
        (394*s, 238*s), (380*s, 96*s), (290*s, 178*s)
    ], fill=black)

    # Rounded head + upper shoulders
    d.ellipse([104*s, 152*s, 408*s, 398*s], fill=black)
    d.rounded_rectangle([140*s, 310*s, 372*s, 484*s], radius=int(72*s), fill=black)

    # Signature chartreuse eyes
    lime = (202, 242, 64, 255)
    d.ellipse([160*s, 232*s, 238*s, 298*s], fill=lime)
    d.ellipse([274*s, 232*s, 352*s, 298*s], fill=lime)

    # Vertical dark pupils
    d.ellipse([190*s, 244*s, 212*s, 286*s], fill=black)
    d.ellipse([300*s, 244*s, 322*s, 286*s], fill=black)

    # Delicate whiskers
    wcol = (225, 228, 232, 200)
    lw = max(1, int(4*s))
    for dy, tilt in [(0, -8), (18, 0), (36, 10)]:
        d.line([(132*s, (306+dy)*s), (76*s, (300+dy+tilt)*s)], fill=wcol, width=lw)
        d.line([(380*s, (306+dy)*s), (436*s, (300+dy+tilt)*s)], fill=wcol, width=lw)

    # Mini keyboard hint at bottom
    d.rounded_rectangle([118*s, 412*s, 394*s, 466*s], radius=int(14*s), fill=(30, 30, 34, 255))
    d.rounded_rectangle([128*s, 420*s, 174*s, 440*s], radius=int(5*s), fill=(246, 176, 39, 255))
    for i in range(5):
        x0 = (182 + i * 38) * s
        d.rounded_rectangle([x0, 420*s, x0 + 30*s, 440*s], radius=int(5*s), fill=(244, 242, 236, 255))
    d.rounded_rectangle([168*s, 444*s, 336*s, 458*s], radius=int(5*s), fill=(244, 242, 236, 255))
    d.rounded_rectangle([344*s, 444*s, 384*s, 458*s], radius=int(5*s), fill=(246, 176, 39, 255))

    return img

for sz in [16, 32, 128, 256, 512]:
    make_icon(sz).save(os.path.join(iconset, f"icon_{sz}x{sz}.png"))
    make_icon(sz * 2).save(os.path.join(iconset, f"icon_{sz}x{sz}@2x.png"))

subprocess.run(["iconutil", "-c", "icns", iconset, "-o", os.path.join(res_dir, "AppIcon.icns")], check=True)
import shutil
shutil.rmtree(iconset)
PYEOF
fi

SWIFT_SOURCES=(
    "$ROOT/Sources/KeyMap.swift"
    "$ROOT/Sources/InputEngine.swift"
    "$ROOT/Sources/CatPhysics.swift"
    "$ROOT/Sources/BeagleAtlas.swift"
    "$ROOT/Sources/CatRenderer.swift"
    "$ROOT/Sources/CoatPatterns.swift"
    "$ROOT/Sources/OverlayWindow.swift"
    "$ROOT/Sources/AppDelegate.swift"
    "$ROOT/Sources/main.swift"
)

COMMON_FLAGS=(
    -swift-version 5
    -Osize
    -framework AppKit
    -framework CoreGraphics
    -framework QuartzCore
    -framework IOKit
    -framework CoreAudio
    -framework ApplicationServices
)

TMP_BIN_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_BIN_DIR"' EXIT

echo "==> Compiling arm64 slice (macOS 11.0+)..."
swiftc "${COMMON_FLAGS[@]}" -target arm64-apple-macos11.0 "${SWIFT_SOURCES[@]}" -o "$TMP_BIN_DIR/${APP_NAME}_arm64"

echo "==> Compiling x86_64 (Intel) slice (macOS 11.0+)..."
swiftc "${COMMON_FLAGS[@]}" -target x86_64-apple-macos11.0 "${SWIFT_SOURCES[@]}" -o "$TMP_BIN_DIR/${APP_NAME}_x86_64"

echo "==> Creating Universal binary (arm64 + x86_64)..."
lipo -create "$TMP_BIN_DIR/${APP_NAME}_arm64" "$TMP_BIN_DIR/${APP_NAME}_x86_64" -output "$MACOS_DIR/$APP_NAME"
strip -x "$MACOS_DIR/$APP_NAME"
lipo -info "$MACOS_DIR/$APP_NAME"

# Keep a consistent signing identity across builds so macOS can match privacy grants.
# Switching identities can require granting Input Monitoring again. Order of preference:
#   1. $KUROCAT_SIGN_IDENTITY (CI imports the shared certificate and sets this)
#   2. Developer ID Application / Apple Development certificate in the keychain
#   3. Self-signed "KuroCat Code Signing" certificate (create once: scripts/create-macos-signing-cert.sh)
#   4. Ad-hoc (last resort — permission will be lost on every update)
SIGN_IDENTITY="${KUROCAT_SIGN_IDENTITY:-}"
if [ -z "$SIGN_IDENTITY" ]; then
    SIGN_IDENTITY="$(security find-identity -v -p codesigning 2>/dev/null | awk -F'"' '/Developer ID Application:/{print $2; exit}')"
fi
if [ -z "$SIGN_IDENTITY" ]; then
    # Prefer the long-lived shared "KuroCat Code Signing" cert (matches CI so permissions survive updates)
    if security find-identity -p codesigning 2>/dev/null | grep -q '"KuroCat Code Signing"'; then
        SIGN_IDENTITY="KuroCat Code Signing"
    fi
fi
if [ -z "$SIGN_IDENTITY" ]; then
    SIGN_IDENTITY="$(security find-identity -v -p codesigning 2>/dev/null | awk -F'"' '/Apple Development:/{print $2; exit}')"
fi

if [ -n "$SIGN_IDENTITY" ]; then
    echo "==> Signing with stable certificate: $SIGN_IDENTITY"
    codesign --force --deep --sign "$SIGN_IDENTITY" --identifier "com.chanke.kurocat" "$APP_BUNDLE"
else
    echo "==> WARNING: no stable signing certificate found — signing ad-hoc." >&2
    echo "    Users will have to re-enable Input Monitoring after EVERY update." >&2
    echo "    Run scripts/create-macos-signing-cert.sh once to fix this." >&2
    if [ -n "${KUROCAT_REQUIRE_STABLE_SIGNATURE:-}" ]; then
        echo "[ERROR] KUROCAT_REQUIRE_STABLE_SIGNATURE is set: refusing to ship an ad-hoc build." >&2
        exit 1
    fi
    codesign --force --deep --sign - --identifier "com.chanke.kurocat" "$APP_BUNDLE"
fi
echo "==> Designated requirement (what macOS uses to recognise KuroCat across updates):"
codesign -d -r- "$APP_BUNDLE" 2>&1 | grep -i designated || true

echo "==> Built successfully: $APP_BUNDLE"
