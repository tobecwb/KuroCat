// Checks CatRenderer.headPoint / bodyPoint (macOS) against tests/rig/golden.tsv.
// Built and run by scripts/check-rig.sh on macOS (needs swiftc); see that script for the source list.
import AppKit
import CoreGraphics

final class RenderSettings {
 static let shared = RenderSettings()
 var coatTheme = CatCoatTheme.classicBlack
 var eyeTheme = EyeColorTheme.neonChartreuse
 var keyboardTheme = KeyboardTheme.classicMilk
 var activeKeyboardSkin: KeyboardSkinSpec { KeyboardSkinSpec.builtIn(keyboardTheme) }
 var showKeyLegends = false
 var overrideSkin: SkinSpec?
 var activeSkin: SkinSpec { overrideSkin ?? SkinSpec.builtIn(coatTheme) }
 let showKeyBadges = false
 let keyboardMarquee = false
 let obsCaptureMode = 0
 let flipHorizontal = false
 let rotationDeg: CGFloat = 0
 let whiteOutline = true
 let overrideSkinIris = false
 let isEditMode = false
 let isEnglish = false
 let scale: CGFloat = 1.0
}

let lines = try! String(contentsOfFile: CommandLine.arguments[1], encoding: .utf8).split(separator: "\n")
let tol: CGFloat = 1e-6
var bad = 0
for line in lines where !line.isEmpty {
    let parts = line.split(separator: "\t")
    let v = parts.dropFirst().map { CGFloat(Double(String($0))!) }
    var skin = SkinSpec.builtIn(.classicBlack)
    let p = CatPhysics()
    let got: CGPoint
    if parts[0] == "head" {
        skin.headSurface = v[3] != 0 ? .dome : .plane
        skin.shape.headScale = v[4]
        p.headYaw.value = v[5]; p.headPitch.value = v[6]; p.headTilt.value = v[7]
        got = CurrentRenderer.headPoint(CGPoint(x: v[0], y: v[1]), skull: v[2] != 0, skin: skin,
                                        head: CGPoint(x: v[8], y: v[9]), physics: p)
    } else {
        skin.shape.bodyWidth = v[2]
        p.bodySquash.value = v[3]; p.lieIntensity.value = v[4]; p.bodyTilt.value = v[5]
        got = CurrentRenderer.bodyPoint(CGPoint(x: v[0], y: v[1]), skin: skin, seatX: v[6], seatY: v[7], physics: p)
    }
    let ex = v[v.count - 2], ey = v[v.count - 1]
    if abs(got.x - ex) > tol || abs(got.y - ey) > tol {
        bad += 1
        if bad <= 10 { print("MISMATCH \(line) -> got (\(got.x), \(got.y))") }
    }
}
if bad > 0 { print("Swift rig: \(bad) of \(lines.count) cases differ"); exit(1) }
print("Swift rig (Sources/CatRenderer.swift): \(lines.count) cases match")
