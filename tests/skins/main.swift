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
let output = URL(fileURLWithPath: CommandLine.arguments[1])
try! FileManager.default.createDirectory(at:output,withIntermediateDirectories:true)
func frame(_ coat: CatCoatTheme,_ eye:EyeColorTheme,_ state:Int,_ legacy:Bool = false,_ pose:Int = 0,_ occludedMuzzle:Bool = false,_ pointyChin:Bool = false)->CGContext {
 let p=CatPhysics()
 switch state {
 case 1: p.headYaw.value = -1; p.headPitch.value = -0.65; p.headTilt.value = -0.13
 case 2: p.headYaw.value = 1; p.headPitch.value = 0.65; p.headTilt.value = 0.13
 case 11:
  p.headYaw.value=CGFloat(pose % 3 - 1);p.headPitch.value=CGFloat(pose / 3 - 1)
 case 10:
  p.headYaw.value=1;p.headPitch.value=1;p.headTilt.value=0.07
  p.eyeGaze.x.value=0.88;p.eyeGaze.y.value=0.85
 case 3:
  p.leftPaw.x.value=78;p.leftPaw.y.value=252;p.leftPawOnMouse=true
  p.rightPaw.x.value=240;p.rightPaw.y.value=268;p.rightPawTyping=true
  p.hindFootActive.value=1;p.hindFoot.x.value=164;p.hindFoot.y.value=270
  p.bodyTilt.value=0.12;p.bodySquash.value=0.90;p.tailPhase=2
 case 8,9:
  p.headYaw.value = -0.65;p.headPitch.value = 0.2;p.headTilt.value = -0.08
  if pose > 0 {
   p.headYaw.value = pose % 2 == 0 ? -1 : 1
   p.headPitch.value = pose % 4 < 2 ? -1 : 1
   p.headTilt.value = pose % 2 == 0 ? -0.18 : 0.18
   p.bodyTilt.value = pose % 2 == 0 ? -0.15 : 0.15
   p.bodySquash.value = pose % 4 < 2 ? 0.85 : 1.08
   p.headOffsetY.value = 14
  }
  if state == 8 {
   p.leftPaw.x.value=78;p.leftPaw.y.value=252;p.leftPawOnMouse=true
   if pose > 3 {p.rightPaw.x.value=240;p.rightPaw.y.value=262;p.rightPawTyping=true}
  }
 case 4:p.sleepiness=1;p.earFold.value=0.25
 case 5:p.petHeadIntensity=0.8;p.earFold.value=0.6
 case 6:p.petBodyIntensity=0.8
 case 7:p.huntIntensity.value=1;p.pupilScale.value=1.12;p.eyeGaze.x.value=0.7;p.eyeGaze.y.value = -0.4
 default:break
 }
 let s=RenderSettings.shared;s.coatTheme=coat;s.eyeTheme=eye
 let ctx=CGContext(data:nil,width:720,height:640,bitsPerComponent:8,bytesPerRow:720*4,space:CGColorSpaceCreateDeviceRGB(),bitmapInfo:CGImageAlphaInfo.premultipliedLast.rawValue)!
 ctx.translateBy(x:0,y:640);ctx.scaleBy(x:2,y:-2)
 if legacy {LegacyRenderer.draw(in:ctx,physics:p,showPermissionPill:false)} else {CurrentRenderer.draw(in:ctx,physics:p,showPermissionPill:false)}
 return ctx
}
func pixels(_ c:CGContext)->[UInt8] {Array(UnsafeBufferPointer(start:c.data!.assumingMemoryBound(to:UInt8.self),count:720*640*4))}
func save(_ c:CGContext,_ name:String){let rep=NSBitmapImageRep(cgImage:c.makeImage()!);try! rep.representation(using:.png,properties:[:])!.write(to:output.appendingPathComponent(name))}
var n=0
let aboveKeyboard = 720 * 460 * 4
for coat in [CatCoatTheme.siamese] {
 for eye in [EyeColorTheme.neonChartreuse,.siameseBlue,.warmAmber,.iceCyan] {
  for state in [0,1,2,4,5,6,7] {
   let current = pixels(frame(coat,eye,state))
   let old = pixels(frame(coat,eye,state,true))
   // Universal sticker-style white outer outline only extends outside the silhouette, and aligned
   // idle front legs share the typing shoulder root (x: 312...423, y >= 352); all remaining opaque
   // pixels above the keyboard (head, ears, face, neck, back, tail) must stay unchanged.
   for i in stride(from: 0, to: aboveKeyboard, by: 4) where old[i + 3] == 255 {
    let px = i / 4
    let x = px % 720, y = px / 720
    if (312...423).contains(x) && y >= 352 { continue }
    precondition(abs(Int(current[i]) - Int(old[i])) <= 2 && abs(Int(current[i+1]) - Int(old[i+1])) <= 2 && abs(Int(current[i+2]) - Int(old[i+2])) <= 2 && current[i+3] == 255,
                 "Existing skin interior changed: \(coat), \(eye), \(state) at \(i)")
   }
   // Compositing cached keyboard bitmaps (plus white paw outline over top keys) preserves keyboard appearance.
   let keyboardDifferences = zip(current[aboveKeyboard...], old[aboveKeyboard...]).reduce(0) {
    $0 + abs(Int($1.0) - Int($1.1))
   }
   precondition(Double(keyboardDifferences) / Double(current.count - aboveKeyboard) < 1.5,
                "Cached keyboard changed appearance: \(coat), \(eye), \(state)")
   n += 1
  }
 }
}
for (name,coat) in [("shorthair",CatCoatTheme.shorthair),("orangeWhite",.orangeWhite)] {
 for state in 0..<8 {
  let c=frame(coat,.referenceHazel,state)
  precondition(pixels(c).filter{$0 != 0}.count>10000,"Empty action render")
  save(c,"\(name)-\(state).png")
 }
}
for (name,coat) in [("black",CatCoatTheme.classicBlack),("siamese",.siamese)] {save(frame(coat,coat == .siamese ? .siameseBlue : .neonChartreuse,0),name+".png")}
save(frame(.shorthair,.softAmber,0),"shorthair-amber.png")
print("PASS: \(n) legacy skin/eye/action frames match upstream v1.3.5 above the keyboard and preserve keyboard appearance; rendered 16 new skin/action frames.")

for (name,coat) in [("shorthair",CatCoatTheme.shorthair),("orangeWhite",.orangeWhite)] {
 save(frame(coat,.softAmber,8),name+"-shoulder-after.png")
 save(frame(coat,.softAmber,8,true),name+"-upstream-shoulder.png")
}

for coat in CatCoatTheme.allCases {
 let active = pixels(frame(coat,.softAmber,8)), idle = pixels(frame(coat,.softAmber,9))
 for y in 160..<316 {
  for x in 240..<490 {
   let i=(y*720+x)*4
   precondition(Array(active[i..<i+4]) == Array(idle[i..<i+4]),"Reaching arm changes face pixels: \(coat)")
  }
 }
}
print("PASS: reaching arms leave the face unchanged across all four skins")

for coat in CatCoatTheme.allCases {
 for pose in 1...8 {
  let active=pixels(frame(coat,.referenceHazel,8,false,pose)),idle=pixels(frame(coat,.referenceHazel,9,false,pose))
  let yaw:CGFloat=pose % 2 == 0 ? -1 : 1, tilt:CGFloat=pose % 2 == 0 ? -0.15 : 0.15
  let pitch:CGFloat=pose % 4 < 2 ? -1 : 1
  let cx=184+tilt*36+yaw*7.5,cy=126+14+pitch*4.8
  for y in Int((cy-35)*2)..<Int((cy+32)*2) {
   for x in Int((cx-52)*2)..<Int((cx+52)*2) {
    let i=(y*720+x)*4
    precondition(Array(active[i..<i+4]) == Array(idle[i..<i+4]),"Arm intrudes on turned face: \(coat), pose \(pose)")
   }
  }
 }
}
print("PASS: 32 extreme turn/tilt/squash poses preserve face pixels during mouse and typing actions")

precondition(EyeColorTheme.saved("淡金青绿（参考图）") == .referenceHazel)
for eye in EyeColorTheme.allCases {precondition(EyeColorTheme.saved(eye.rawValue) == eye)}
print("PASS: renamed hazel theme restores both old and current saved settings")

let swatch=CGContext(data:nil,width:1,height:1,bitsPerComponent:8,bytesPerRow:4,space:CGColorSpaceCreateDeviceRGB(),bitmapInfo:CGImageAlphaInfo.premultipliedLast.rawValue)!
swatch.setFillColor(NSColor(calibratedRed:216.0/255,green:170.0/255,blue:115.0/255,alpha:1).cgColor)
swatch.fill(CGRect(x:0,y:0,width:1,height:1))
let ginger=Array(UnsafeBufferPointer(start:swatch.data!.assumingMemoryBound(to:UInt8.self),count:4))
for pose in 1...8 {
 let buffer=pixels(frame(.orangeWhite,.referenceHazel,9,false,pose))
 let yaw:CGFloat=pose % 2 == 0 ? -1 : 1,tilt:CGFloat=pose % 2 == 0 ? -0.15 : 0.15
 let pitch:CGFloat=pose % 4 < 2 ? -1 : 1,roll:CGFloat=pose % 2 == 0 ? -0.18 : 0.18
 let cx=184+tilt*36+yaw*7.5,cy=126+14+pitch*4.8
 let x = -yaw*38,y:CGFloat = -8
 let px=Int((cx+x*cos(roll)-y*sin(roll))*2),py=Int((cy+x*sin(roll)+y*cos(roll))*2)
 let offset=(py*720+px)*4
 for channel in 0..<4 {precondition(abs(Int(buffer[offset+channel])-Int(ginger[channel])) <= 2,"Orange-white rear head exposed in pose \(pose)")}
}
print("PASS: orange-white rear head remains orange through left/right, pitch and roll poses")

// Turned-down cheek intersected a body spot in v1.3.3; sample its interior, not an antialiased edge.
swatch.setFillColor(NSColor(calibratedRed:242.0/255,green:238.0/255,blue:228.0/255,alpha:1).cgColor)
swatch.fill(CGRect(x:0,y:0,width:1,height:1))
let milk=Array(UnsafeBufferPointer(start:swatch.data!.assumingMemoryBound(to:UInt8.self),count:4))
let chin=pixels(frame(.orangeWhite,.referenceHazel,2))
// Track material points on the moving cheek instead of fixed screen pixels:
// changing its smooth depth legitimately moves the cheek on screen.
let chinPose=CatPhysics();chinPose.headYaw.value=1;chinPose.headPitch.value=0.65;chinPose.headTilt.value=0.13
let chinCenter=CGPoint(x:191.5,y:126+0.65*4.8)
for y in 24...26 {for x in 10...12 {
 let pt=CurrentRenderer.coatMarkPoint(CGPoint(x:x,y:y),headSurface:true,skull:false,coat:.orangeWhite,seatX:184,seatY:226,head:chinCenter,physics:chinPose,whiteSurface:true)
 let i=(Int(pt.y*2)*720+Int(pt.x*2))*4
 for ch in 0..<4 {precondition(abs(Int(chin[i+ch])-Int(milk[ch]))<=2,"Body spot visible through cheek material")}
}}
print("PASS: 9 cheek material points remain opaque white")

let neck=CoatPatterns.orangeWhite.first{$0.head && !$0.skull && $0.rgb==0xF2EEE4 && $0.points.contains{abs($0.x)>46}}!
for side:CGFloat in [-1,1] {
 let pose=CatPhysics();pose.headYaw.value=side
 var edge=neck.points.filter{$0.x*side>=29 && $0.y>=20 && $0.y<=28}
 if side>0 {edge.reverse()}
 var last:CGFloat = -CGFloat.infinity
 for pt in edge {
  let q=CurrentRenderer.coatMarkPoint(pt,headSurface:true,skull:false,coat:.orangeWhite,seatX:0,seatY:0,head:.zero,physics:pose,whiteSurface:true).x*side
  precondition(q>=last-0.001,"White neck contour folds into an orange point")
  last=q
 }
}
print("PASS: left/right white cheek rim has no contour folds")

for coat in [CatCoatTheme.shorthair,.orangeWhite] {
 let marks=CoatPatterns.marks(for:coat)
 let lastBody=marks.lastIndex(where:{!$0.head})!
 let firstPattern=marks.firstIndex(where:{$0.head && !$0.skull && $0.rgb != 0xF2EEE4})!
 precondition(firstPattern>lastBody,"Body element painted above head pattern")
 let face=marks.last!
 precondition(face.head && !face.skull && face.rgb==0xF2EEE4 && face.alpha==1 && face.width==0,"Cheek/chin must be the opaque foreground coat layer")
}
func cheekPath(_ points:[CGPoint])->CGPath {
 let p=CGMutablePath();p.addLines(between:points);p.closeSubpath();return p
}
let roundedCheek=cheekPath(CoatPatterns.shorthair.last!.points)
precondition(roundedCheek.contains(CGPoint(x:0,y:34)),"Rounded chin lost its central arc")
precondition(!roundedCheek.contains(CGPoint(x:14,y:34)),"Shorthair lower chin still has a flat wide edge")
print("PASS: all body marks behind head patterns; opaque foreground cheeks; small rounded shorthair chin")
for coat in [CatCoatTheme.shorthair,.orangeWhite] {
 let now=CoatPatterns.marks(for:coat)
 let old=BaselinePatterns.marks(for:coat)
 let body=now.filter{!$0.head},oldBody=old.filter{!$0.head}
 precondition(body.count==oldBody.count)
 for (index,pair) in zip(body,oldBody).enumerated() {
 let (a,b)=pair
 if coat == .orangeWhite && index == 0 {
  precondition(Array(a.points.prefix(25)) == Array(b.points.prefix(25)),"Rounded inner body-spot boundary changed")
  precondition(a.points.map{$0.x}.max()! > 80,"Outer orange spot does not bleed beyond silhouette")
  continue
 }
 precondition(a.points==b.points && a.rgb==b.rgb && a.width==b.width && a.alpha==b.alpha,"Original body coat artwork changed")}
 let patterns=now.filter{$0.head && !$0.skull && $0.rgb != 0xF2EEE4}
 let oldPatterns=old.filter{$0.head && !$0.skull && $0.rgb != 0xF2EEE4}
 precondition(patterns.count==oldPatterns.count)
 for (a,b) in zip(patterns,oldPatterns) {precondition(a.points==b.points && a.rgb==b.rgb && a.width==b.width && a.alpha==b.alpha,"Original head pattern changed")}
}
print("PASS: rounded body-spot inner edges, tabby stripes and forehead preserved; right outer spot covers silhouette")

let rightDown=pixels(frame(.orangeWhite,.referenceHazel,10))
for y in 313...315 {for x in 429...431 {
 let i=(y*720+x)*4
 for ch in 0..<4 {precondition(abs(Int(rightDown[i+ch])-Int(milk[ch]))<=2,"Body spot exposed in extreme right-down cheek overlap")}
}}
print("PASS: extreme right-down face covers all 9 formerly exposed body-spot pixels")

let frontBody=pixels(frame(.orangeWhite,.referenceHazel,0))
for y in 313...315 {for x in 430...432 {
 let i=(y*720+x)*4
 for ch in 0..<4 {precondition(abs(Int(frontBody[i+ch])-Int(ginger[ch]))<=2,"Extra chin coverage hides front-facing body spot")}
}}
print("PASS: front-facing body spot remains visible while extreme right-down cheek stays opaque")

RenderSettings.shared.showKeyLegends = false
let withoutLegends = pixels(frame(.orangeWhite,.referenceHazel,0))
RenderSettings.shared.showKeyLegends = true
let withLegends = pixels(frame(.orangeWhite,.referenceHazel,0))
precondition(withoutLegends != withLegends,"Keyboard legend toggle has no effect")
for y in 160..<342 { for x in 240..<490 { let i=(y*720+x)*4
 precondition(Array(withoutLegends[i..<i+4]) == Array(withLegends[i..<i+4]),"Keyboard legends change face artwork")
}}
RenderSettings.shared.showKeyLegends = false
print("PASS: keyboard legend toggle changes keys without changing face pixels")

// The nose and both mouth lobes stay visible at all nine full yaw/pitch poses.
func hasInk(_ data:[UInt8],_ at:CGPoint,_ nose:Bool)->Bool {
 let px=Int(at.x*2),py=Int(at.y*2)
 for y in (py-2)...(py+2) {for x in (px-2)...(px+2) {
  let i=(y*720+x)*4; let rgb=Array(data[i..<i+3]).map{Int($0)}
  if nose && rgb[0]>200 && rgb[0]-rgb[1]>35 && rgb[2]-rgb[1]>5 && rgb[2]<200 {return true}
  if !nose && rgb.min()! > 85 && rgb.max()! < 205 && rgb.max()!-rgb.min()! < 20 {return true}
 }}
 return false
}
for (name,coat) in [("shorthair",CatCoatTheme.shorthair),("orangeWhite",.orangeWhite)] {
 for pose in 0..<9 {
  let yaw=CGFloat(pose % 3 - 1),pitch=CGFloat(pose / 3 - 1)
  let c=frame(coat,.referenceHazel,11,false,pose),data=pixels(c)
  let center=CGPoint(x:184+yaw*7.5,y:126+pitch*4.8)
  func location(_ x:CGFloat,_ y:CGFloat)->CGPoint {
   let p=CurrentRenderer.projectHead3D(x:x,y:y,z:30,yawRad:yaw*0.5,pitchRad:pitch*0.36).pt
   return CGPoint(x:center.x+p.x,y:center.y+p.y)
  }
  precondition(hasInk(data,location(0,9.8),true),"Nose lost at pose \(pose), \(coat)")
  precondition(hasInk(data,location(-3.025,17.0875),false),"Left mouth lost at pose \(pose), \(coat)")
  precondition(hasInk(data,location(3.025,17.0875),false),"Right mouth lost at pose \(pose), \(coat)")
  save(c,"\(name)-muzzle-view-\(pose).png")
 }
}
print("PASS: noses and both mouth lobes visible in all 18 extreme yaw/pitch renders")

// MARK: - .kurocat skins (docs/skin-format.md)
let repoRoot = ProcessInfo.processInfo.environment["KUROCAT_ROOT"].map { URL(fileURLWithPath: $0) }
    ?? URL(fileURLWithPath: #filePath).deletingLastPathComponent().deletingLastPathComponent().deletingLastPathComponent()
let templates: [(String, CatCoatTheme, EyeColorTheme)] = [("black", .classicBlack, .neonChartreuse), ("siamese", .siamese, .siameseBlue),
                                                          ("shorthair", .shorthair, .softAmber), ("orangeWhite", .orangeWhite, .referenceHazel)]
for (file, coat, eye) in templates {
 let spec = try! SkinSpec.load(contentsOf: repoRoot.appendingPathComponent("skins/\(file).kurocat"))
 precondition(spec.name == SkinSpec.builtIn(coat).name, "Template name mismatch: \(file)")
 for state in [0, 2, 3, 7] {
  RenderSettings.shared.overrideSkin = nil
  let builtIn = pixels(frame(coat, eye, state))
  RenderSettings.shared.overrideSkin = spec
  let imported = pixels(frame(coat, eye, state))
  var far = 0
  for i in stride(from: 0, to: builtIn.count, by: 4) where (0..<4).contains(where: { abs(Int(builtIn[i + $0]) - Int(imported[i + $0])) > 6 }) { far += 1 }
  precondition(far < 200, "Template \(file) drifts from the built-in coat in state \(state): \(far) px")
 }
}
let demo = try! SkinSpec.load(contentsOf: repoRoot.appendingPathComponent("skins/samples/curve-regions-demo.kurocat"))
precondition(demo.regions.count >= 3 && demo.shape.bodyWidth > 1.2, "Sample skin lost its curve regions / chubby shape")
RenderSettings.shared.overrideSkin = demo
for state in [0, 1, 2, 3, 8] {
 let c = frame(.classicBlack, .softAmber, state, false, state == 8 ? 3 : 0)
 precondition(pixels(c).filter { $0 != 0 }.count > 10000, "Empty sample-skin render")
 save(c, "curve-regions-demo-\(state).png")
}
RenderSettings.shared.overrideSkin = nil
func rejects(_ json: String, _ fragment: String) {
 do { _ = try SkinSpec.parse(Data(json.utf8)); preconditionFailure("Accepted invalid skin: \(json)") }
 catch { precondition(error.localizedDescription.contains(fragment), "Unexpected error for \(json): \(error.localizedDescription)") }
}
rejects("{", "JSON")
rejects("[]", "JSON 对象")
rejects(##"{"name":"x","palette":{"body":"#000000"}}"##, "formatVersion")
rejects(##"{"formatVersion":3,"name":"x","palette":{"body":"#000000"}}"##, "升级")
rejects(##"{"formatVersion":2,"minAppVersion":"bad","name":"x","palette":{"body":"#000000"}}"##, "minAppVersion")
precondition(SkinSpec.compareVersions("1.5.3", "1.5.2") > 0 && SkinSpec.compareVersions("1.5.10", "1.5.9") > 0, "compareVersions broken")
_ = try! SkinSpec.parse(Data(##"{"formatVersion":2,"minAppVersion":"1.5.3","name":"v2","palette":{"body":"#000000"}}"##.utf8))
rejects(##"{"formatVersion":1,"name":"x","palette":{"body":"pink"}}"##, "palette.body")
rejects(##"{"formatVersion":1,"name":"x","palette":{}}"##, "palette.body")
rejects(##"{"formatVersion":1,"name":"x","palette":{"body":"#000000"},"regions":[{"space":123,"fill":"#FFFFFF","anchors":[]}]}"##, "regions[0].space")
rejects(##"{"formatVersion":1,"name":"x","palette":{"body":"#000000"},"shape":{"bodyWidth":"fat"}}"##, "shape.bodyWidth")
let clamped = try! SkinSpec.parse(Data(##"{"formatVersion":1,"name":"  胖  ","palette":{"body":"#112233","hindLeg":"#223344","hindPaw":"#334455"},"shape":{"bodyWidth":9},"unknownField":true,"marks":[{"space":"hindLeg","kind":"polygon","color":"#FFFFFF","points":[[10,0],[50,5],[80,-5]]},{"space":"wing","kind":"polygon","color":"#FFFFFF","points":[[10,0],[50,5],[80,-5]]}]}"##.utf8))
precondition(clamped.shape.bodyWidth == 1.35 && clamped.name == "胖", "Clamping / trimming / unknown-field rules broken")
precondition(clamped.marks.count == 1 && clamped.marks[0].space == .hindLeg && clamped.palette.hindLeg != nil && clamped.palette.hindPaw != nil, "hindLeg support / forward-compat unknown-space skip broken")
print("PASS: 4 .kurocat templates match their built-in coats, curve-regions sample renders, invalid files are rejected in Chinese")

// MARK: - A sleeping cat lying still is not animating
let sitting=CatPhysics()
precondition(!sitting.isChangingLie,"A sitting cat counts as changing its lie")
let lyingStill=CatPhysics();lyingStill.lieIntensity.value=1;lyingStill.lieIntensity.target=1
precondition(!lyingStill.isChangingLie,"A cat lying still counts as changing its lie")
let lyingDown=CatPhysics();lyingDown.lieIntensity.value=0.4;lyingDown.lieIntensity.target=1
precondition(lyingDown.isChangingLie,"Lying down does not count as changing the lie")
let sittingUp=CatPhysics();sittingUp.lieIntensity.value=0.7;sittingUp.lieIntensity.target=0
precondition(sittingUp.isChangingLie,"Sitting up does not count as changing the lie")
let settling=CatPhysics();settling.lieIntensity.value=1;settling.lieIntensity.target=1;settling.lieIntensity.velocity=0.3
precondition(settling.isChangingLie,"A lying cat still in motion does not count as changing the lie")
print("PASS: lying down, sitting up and settling count as lie changes; sitting or lying still does not")
