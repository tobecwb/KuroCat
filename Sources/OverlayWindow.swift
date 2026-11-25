import Foundation
import AppKit
import UniformTypeIdentifiers

private struct RenderedPose {
    struct ForegroundAppearance: Equatable {
        let coat: CatCoatTheme
        let skinId: String
        let eyes: EyeColorTheme
        let tabletKeyboard: KeyboardTheme?
        let size: CGSize
        let flipped: Bool
        let rotationDeg: CGFloat
        let whiteOutline: Bool
        let overrideSkinIris: Bool
        let tabletActive: Bool
    }

    struct Appearance: Equatable {
        let foreground: ForegroundAppearance
        let keyboard: KeyboardTheme
        let keyboardSkinId: String
        let showsLegends: Bool
        let levelKeyboard: Bool
        let captureMode: Int
        let showEditControls: Bool
        let englishLabels: Bool
    }

    let appearance: Appearance
    let marqueeBucket: Int
    let hasEffects: Bool
    let head: CGPoint
    let tail: CGPoint
    let tailBend: CGPoint
    let leftPaw: CGPoint
    let rightPaw: CGPoint
    let eyeClosure: CGFloat
    let headTurn: CGPoint
    let headTilt: CGFloat
    let eyeGaze: CGPoint
    let mouseOffset: CGPoint
    let mousePressLeft: CGFloat
    let mousePressRight: CGFloat
    let mousePressMiddle: CGFloat
    let pawOnMouse: Bool
    let hindFoot: CGPoint
    let hindFootActive: CGFloat
    let yawnAmount: CGFloat
    let earFold: CGFloat
    let earTwitch: CGFloat
    let pupilScale: CGFloat
    let bodyTilt: CGFloat
    let bodySquash: CGFloat
    let lieIntensity: CGFloat
    let anger: CGFloat
    let dizzy: CGFloat
    let petIntensity: CGFloat
    let douniLoaf: CGFloat
    let douniMouse: CGFloat
    let douniLeftRaise: CGFloat
    let douniRightRaise: CGFloat
    let douniLeftTarget: CGPoint
    let douniRightTarget: CGPoint
    let douniBreath: CGFloat
    let douniAccentBucket: Int
    let hunt: CGFloat
    let tabletNib: CGPoint
    let tabletPress: CGFloat
    let toastActive: Bool
    let toastProgress: CGFloat
    let showPermissionPill: Bool
    let permissionPillText: String

    init(physics: CatPhysics, settings: CatSettings, skin: SkinSpec, size: CGSize,
         showPermissionPill: Bool, permissionPillText: String,
         toastActive: Bool = false, toastProgress: CGFloat = 0) {
        let coat = settings.coatTheme
        let tabletActive = physics.isTabletActive
        let kbTheme = settings.keyboardTheme
        let fgAppearance = ForegroundAppearance(
            coat: coat,
            skinId: skin.id,
            eyes: settings.eyeTheme,
            tabletKeyboard: tabletActive ? kbTheme : nil,
            size: size,
            flipped: settings.flipHorizontal,
            rotationDeg: settings.rotationDeg,
            whiteOutline: settings.whiteOutline,
            overrideSkinIris: settings.overrideSkinIris,
            tabletActive: tabletActive
        )
        appearance = Appearance(
            foreground: fgAppearance,
            keyboard: kbTheme,
            keyboardSkinId: settings.activeKeyboardSkin.id,
            showsLegends: settings.showKeyLegends,
            levelKeyboard: settings.levelKeyboard,
            captureMode: settings.obsCaptureMode,
            showEditControls: settings.isEditMode,
            englishLabels: settings.isEnglish
        )

        let scruff = physics.scruffDrag.value
        let loaf = physics.douniLoafBlend.value
        let asleep = physics.sleepiness >= 0.95
        let keyboardVisible = (skin.renderStyle == .douni) ? (loaf < 0.99 && scruff < 0.99) : (scruff * 2.6 < 0.99)
        marqueeBucket = (settings.keyboardMarquee && keyboardVisible)
            ? CatRenderer.marqueeBucket(phase: physics.marqueePhase, asleep: asleep)
            : -1

        let pet = max(physics.petHeadIntensity, physics.petBodyIntensity)
        hasEffects = !physics.sparkles.isEmpty
            || !physics.floatingKeycaps.isEmpty || !physics.keyGlow.isEmpty
            || !physics.tabletTrail.isEmpty || physics.kbSparkTimer > 0.005
            || physics.angerIntensity.value > 0.02 || physics.dizzyIntensity.value > 0.02
            || physics.scruffDizzyIntensity.value > 0.02
            || physics.huntIntensity.value > 0.02
            || scruff > 0.02 || physics.isChangingLie
            || abs(physics.scruffSwing.value) > 0.01 || abs(physics.scruffSwingY.value) > 0.01
            || abs(physics.scruffTailSwing.value) > 0.01
            || (loaf > 0.005 && loaf < 0.995)
            || abs(physics.douniLoafBlend.velocity) > 0.02
            || abs(physics.douniLeftRaise.velocity) > 0.05 || abs(physics.douniRightRaise.velocity) > 0.05
            || abs(physics.douniMouseBlend.velocity) > 0.05
            || abs(physics.kbImpactY.value) > 0.01 || abs(physics.kbImpactTilt.value) > 0.01
            || pet > 0.02
            || toastActive

        head = CatRenderer.headPosition(physics, skin: skin)
        let tailCurve = CatRenderer.tailCurve(physics)
        tail = tailCurve.tip
        tailBend = tailCurve.control2
        leftPaw = physics.leftPaw.point
        rightPaw = physics.rightPaw.point
        eyeClosure = physics.effectiveEyeClosure
        headTurn = CGPoint(x: physics.headYaw.value, y: physics.headPitch.value)
        headTilt = physics.headTilt.value
        eyeGaze = physics.eyeGaze.point
        mouseOffset = physics.mouseOffset.point
        mousePressLeft = physics.mousePressLeft.value
        mousePressRight = physics.mousePressRight.value
        mousePressMiddle = physics.mousePressMiddle.value
        pawOnMouse = physics.leftPawOnMouse
        hindFoot = physics.hindFoot.point
        hindFootActive = physics.hindFootActive.value
        yawnAmount = physics.yawnAmount
        earFold = physics.earFold.value
        earTwitch = physics.earTwitch.value
        pupilScale = physics.pupilScale.value
        bodyTilt = physics.bodyTilt.value
        bodySquash = physics.bodySquash.value
        lieIntensity = physics.lieIntensity.value
        anger = physics.angerIntensity.value
        dizzy = max(physics.dizzyIntensity.value, physics.scruffDizzyIntensity.value)
        petIntensity = pet
        douniLoaf = loaf
        douniMouse = physics.douniMouseBlend.value
        douniLeftRaise = physics.douniLeftRaise.value
        douniRightRaise = physics.douniRightRaise.value
        douniLeftTarget = physics.douniLeftKeyTarget.point
        douniRightTarget = physics.douniRightKeyTarget.point
        if skin.renderStyle == .douni && loaf > 0.85 {
            douniBreath = sin(physics.time * (asleep ? 1.7 : 2.3))
            douniAccentBucket = (asleep || pet > 0.05) ? Int((physics.time * (asleep ? 15.0 : 24.0)).rounded()) : 0
        } else {
            douniBreath = 0
            douniAccentBucket = 0
        }
        hunt = physics.huntIntensity.value
        tabletNib = physics.tabletNib.point
        tabletPress = physics.tabletPenPress.value
        self.toastActive = toastActive
        self.toastProgress = toastProgress
        self.showPermissionPill = showPermissionPill
        self.permissionPillText = showPermissionPill ? permissionPillText : ""
    }

    func foregroundDiffers(from previous: RenderedPose) -> Bool {
        if appearance.foreground != previous.appearance.foreground || hasEffects || previous.hasEffects
            || pawOnMouse != previous.pawOnMouse || douniAccentBucket != previous.douniAccentBucket {
            return true
        }
        func distance(_ a: CGPoint, _ b: CGPoint) -> CGFloat { hypot(a.x - b.x, a.y - b.y) }
        // Estimate how far visible features moved in canvas coordinates, then convert to screen points.
        let movement = max(distance(head, previous.head),
                           distance(tail, previous.tail),
                           distance(tailBend, previous.tailBend),
                           distance(leftPaw, previous.leftPaw),
                           distance(rightPaw, previous.rightPaw),
                           abs(eyeClosure - previous.eyeClosure) * 40,
                           distance(headTurn, previous.headTurn) * 24,
                           abs(headTilt - previous.headTilt) * 52,
                           distance(eyeGaze, previous.eyeGaze) * 10,
                           distance(mouseOffset, previous.mouseOffset),
                           distance(hindFoot, previous.hindFoot) * max(hindFootActive, previous.hindFootActive),
                           abs(hindFootActive - previous.hindFootActive) * 30,
                           abs(yawnAmount - previous.yawnAmount) * 25,
                           abs(earFold - previous.earFold) * 25,
                           abs(earTwitch - previous.earTwitch) * 18,
                           abs(pupilScale - previous.pupilScale) * 20,
                           abs(bodyTilt - previous.bodyTilt) * 45,
                           abs(bodySquash - previous.bodySquash) * 40,
                           abs(lieIntensity - previous.lieIntensity) * 60,
                           abs(anger - previous.anger) * 20,
                           abs(dizzy - previous.dizzy) * 25,
                           abs(petIntensity - previous.petIntensity) * 15,
                           abs(douniLoaf - previous.douniLoaf) * 90,
                           abs(douniMouse - previous.douniMouse) * 50,
                           abs(douniLeftRaise - previous.douniLeftRaise) * 55,
                           abs(douniRightRaise - previous.douniRightRaise) * 55,
                           distance(douniLeftTarget, previous.douniLeftTarget) * 0.4,
                           distance(douniRightTarget, previous.douniRightTarget) * 0.4,
                           abs(douniBreath - previous.douniBreath) * 15,
                           abs(hunt - previous.hunt) * 40,
                           distance(tabletNib, previous.tabletNib),
                           abs(tabletPress - previous.tabletPress) * 20)
        let scale = max(appearance.foreground.size.width / CatPhysics.canvasSize.width,
                        appearance.foreground.size.height / CatPhysics.canvasSize.height)
        return movement * scale >= 0.65
    }

    func visiblyDiffers(from previous: RenderedPose) -> Bool {
        if marqueeBucket != previous.marqueeBucket
            || appearance != previous.appearance
            || toastActive != previous.toastActive
            || abs(toastProgress - previous.toastProgress) > 0.004
            || showPermissionPill != previous.showPermissionPill
            || permissionPillText != previous.permissionPillText {
            return true
        }
        let scale = max(appearance.foreground.size.width / CatPhysics.canvasSize.width,
                        appearance.foreground.size.height / CatPhysics.canvasSize.height)
        let pressMovement = max(abs(mousePressLeft - previous.mousePressLeft),
                                abs(mousePressRight - previous.mousePressRight),
                                abs(mousePressMiddle - previous.mousePressMiddle)) * 20
        if pressMovement * scale >= 0.65 {
            return true
        }
        return foregroundDiffers(from: previous)
    }
}

final class CatCanvasView: NSView {
    private static let fgColorSpace = CGColorSpaceCreateDeviceRGB()
    private enum ActiveCursor { case arrow, openHand, pointingHand, crosshair, closedHand }

    let physics = CatPhysics()
    var onRequestPermission: (() -> Void)?
    var onRightClickMenu: ((NSEvent) -> Void)?
    var onUserInteraction: (() -> Void)?
    /// A `.kurocat` dropped onto the cat is imported and worn right away.
    var onDropSkinFiles: (([URL]) -> Void)?

    /// Extra transparent space reserved above the cat canvas, so the hourly
    /// toast pill (60pt above the head) never gets clipped by the view bounds.
    static let contentTopInset: CGFloat = 64

    private var lastHoverCanvasPt: CGPoint?
    private var isDragging = false
    private var isRotating = false
    private var rotateStartMouseAngle: CGFloat = 0
    private var rotateStartDeg: CGFloat = 0
    private var rotateDragged = false
    private var activeResizeCorner: Int = -1 // -1 = none, 0=TL, 1=TR, 2=BL, 3=BR
    private var resizeAnchorScreenPt: CGPoint = .zero
    private var resizeStartFrame: CGRect = .zero
    private var dragStartScreenPt: CGPoint = .zero
    private var dragStartWindowOrigin: CGPoint = .zero
    private var activeCursor: ActiveCursor = .arrow
    private var lastHoverWasInside = false
    private var lastDrawnPose: RenderedPose?
    private var cachedForegroundImage: CGImage?
    private var cachedForegroundPose: RenderedPose?
    private var cachedForegroundBackingScale: CGFloat = 0

    // MARK: - Toast (drawn directly on the canvas, above the cat's head)
    private var toastText: String? = nil
    private var toastProgress: CGFloat = 0
    private var toastPhase: Double = 0
    private let toastFadeIn: Double = 0.4
    private let toastHold: Double = 4.2
    private let toastFadeOut: Double = 0.4
    var toastActive: Bool { toastText != nil }

    var isDraggingOrResizing: Bool { isDragging || isRotating || activeResizeCorner >= 0 }

    override var isFlipped: Bool { true }
    override var isOpaque: Bool { false }
    override var acceptsFirstResponder: Bool { false }

    override init(frame frameRect: NSRect) {
        super.init(frame: frameRect)
        wantsLayer = true
        layerContentsRedrawPolicy = .onSetNeedsDisplay
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        wantsLayer = true
        layerContentsRedrawPolicy = .onSetNeedsDisplay
    }

    override func draw(_ dirtyRect: NSRect) {
        guard let ctx = NSGraphicsContext.current?.cgContext else { return }
        ctx.clear(bounds)

        let totalCanvasH = CatPhysics.canvasSize.height + CatCanvasView.contentTopInset
        let scaleX = bounds.width / CatPhysics.canvasSize.width
        let scaleY = bounds.height / totalCanvasH
        ctx.saveGState()
        ctx.scaleBy(x: scaleX, y: scaleY)
        // Shift the cat down, leaving transparent headroom above for the toast
        ctx.translateBy(x: 0, y: CatCanvasView.contentTopInset)

        let settings = CatSettings.shared
        let skin = settings.activeSkin
        let status = InputEngine.shared.status
        let showPill = status.needsAttention
        let pillText = status.permissionPillText
        let currentPose = RenderedPose(physics: physics, settings: settings, skin: skin, size: bounds.size,
                                       showPermissionPill: showPill, permissionPillText: pillText,
                                       toastActive: toastActive, toastProgress: toastProgress)

        if physics.sleepiness >= 0.95,
           CatRenderer.canSplitLayers(physics: physics, skin: skin),
           settings.keyboardMarquee,
           !currentPose.hasEffects {
            let backingScale = window?.backingScaleFactor ?? NSScreen.main?.backingScaleFactor ?? 2.0
            let needRebuild: Bool = {
                guard cachedForegroundImage != nil,
                      let fgPose = cachedForegroundPose,
                      cachedForegroundBackingScale == backingScale else { return true }
                return currentPose.foregroundDiffers(from: fgPose)
            }()
            if needRebuild {
                cachedForegroundImage = renderForegroundBitmap(backingScale: backingScale)
                cachedForegroundPose = currentPose
                cachedForegroundBackingScale = backingScale
            }
            if let fgImage = cachedForegroundImage {
                CatRenderer.drawBackdrop(in: ctx, physics: physics)
                ctx.saveGState()
                ctx.translateBy(x: 0, y: CatPhysics.canvasSize.height)
                ctx.scaleBy(x: 1, y: -1)
                ctx.draw(fgImage, in: CGRect(origin: .zero, size: CatPhysics.canvasSize))
                ctx.restoreGState()
                CatRenderer.drawOverlays(in: ctx, showPermissionPill: showPill, permissionPillText: pillText)
                if toastActive, let text = toastText {
                    drawToastPill(in: ctx, text: text, alpha: toastProgress)
                }
                lastDrawnPose = currentPose
                ctx.restoreGState()
                return
            }
        }

        cachedForegroundImage = nil
        cachedForegroundPose = nil
        CatRenderer.draw(in: ctx, physics: physics, showPermissionPill: showPill,
                         permissionPillText: pillText)
        if toastActive, let text = toastText {
            drawToastPill(in: ctx, text: text, alpha: toastProgress)
        }
        lastDrawnPose = currentPose
        ctx.restoreGState()
    }

    private func renderForegroundBitmap(backingScale: CGFloat) -> CGImage? {
        let scale = bounds.width / CatPhysics.canvasSize.width
        let pxW = max(1, Int((CatPhysics.canvasSize.width * scale * backingScale).rounded()))
        let pxH = max(1, Int((CatPhysics.canvasSize.height * scale * backingScale).rounded()))
        guard let offCtx = CGContext(data: nil, width: pxW, height: pxH,
                                     bitsPerComponent: 8, bytesPerRow: 0,
                                     space: Self.fgColorSpace,
                                     bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue) else { return nil }
        offCtx.translateBy(x: 0, y: CGFloat(pxH))
        offCtx.scaleBy(x: CGFloat(pxW) / CatPhysics.canvasSize.width,
                       y: -CGFloat(pxH) / CatPhysics.canvasSize.height)
        CatRenderer.drawForeground(in: offCtx, physics: physics)
        return offCtx.makeImage()
    }

    func poseNeedsDisplay() -> Bool {
        guard let previous = lastDrawnPose else { return true }
        let settings = CatSettings.shared
        let status = InputEngine.shared.status
        let current = RenderedPose(physics: physics, settings: settings, skin: settings.activeSkin, size: bounds.size,
                                   showPermissionPill: status.needsAttention,
                                   permissionPillText: status.permissionPillText,
                                   toastActive: toastActive, toastProgress: toastProgress)
        return current.visiblyDiffers(from: previous)
    }

    func canvasPoint(fromScreenPoint screenPt: CGPoint) -> CGPoint? {
        guard let win = window else { return nil }
        let frame = win.frame
        guard frame.contains(screenPt) else { return nil }
        let localX = (screenPt.x - frame.minX) / frame.width * CatPhysics.canvasSize.width
        // Convert bottom-up screen Y to flipped top-down canvas Y (minus the top headroom)
        let totalCanvasH = CatPhysics.canvasSize.height + CatCanvasView.contentTopInset
        let localY = (frame.maxY - screenPt.y) / frame.height * totalCanvasH - CatCanvasView.contentTopInset
        return CGPoint(x: localX, y: localY)
    }

    /// Updates smart click-through, cursor shape, and head petting strokes.
    func updateHoverAndClickThrough() {
        guard let win = window else { return }
        if isDraggingOrResizing {
            if win.ignoresMouseEvents {
                win.ignoresMouseEvents = false
            }
            return
        }

        let screenPt = NSEvent.mouseLocation
        guard let pt = canvasPoint(fromScreenPoint: screenPt) else {
            if lastHoverWasInside || lastHoverCanvasPt != nil {
                lastHoverWasInside = false
                lastHoverCanvasPt = nil
                physics.updateTailHover(canvasPoint: nil)
            }
            if !win.ignoresMouseEvents {
                win.ignoresMouseEvents = true
            }
            setCursorIfNeeded(.arrow)
            return
        }
        lastHoverWasInside = true

        if CatSettings.shared.isEditMode {
            let editHit = CatRenderer.hitTestEditOverlay(point: pt)
            let shouldIgnore = (editHit < 0)
            if win.ignoresMouseEvents != shouldIgnore {
                win.ignoresMouseEvents = shouldIgnore
            }
            if editHit == 20 {
                setCursorIfNeeded(.pointingHand)
            } else if editHit >= 0 && editHit <= 3 {
                setCursorIfNeeded(.crosshair)
            } else if editHit == 12 {
                setCursorIfNeeded(.openHand)
            } else {
                setCursorIfNeeded(.arrow)
            }
            return
        }

        let catPt = CatRenderer.canvasPointToCatSpace(pt, flipped: CatSettings.shared.flipHorizontal, rotationDeg: CatSettings.shared.rotationDeg)
        let showPill = InputEngine.shared.status.needsAttention
        let zone = CatRenderer.hitTest(point: catPt, physics: physics, showPermissionPill: showPill)
        physics.updateTailHover(canvasPoint: catPt)

        // Petting triggers strictly on the cat's head (never on keyboard or torso)!
        if zone == .head {
            let skipPetForHunt = (CatSettings.shared.activeSkin.renderStyle == .douni && physics.huntIntensity.value > 0.20)
            if !skipPetForHunt, let prev = lastHoverCanvasPt {
                let delta = hypot(catPt.x - prev.x, catPt.y - prev.y)
                if delta > 1.2 && delta < 80 {
                    physics.registerPetStroke(canvasPoint: catPt, delta: delta, isHead: true)
                    onUserInteraction?()
                }
            }
            lastHoverCanvasPt = catPt
        } else {
            lastHoverCanvasPt = nil
        }

        // Smart Click-Through logic (hold Option ⌥ to temporarily unlock Full Click-Through!)
        let optionHeld = NSEvent.modifierFlags.contains(.option)
        if CatSettings.shared.fullClickThrough && !optionHeld {
            // Keep permission pill clickable only if permission isn't granted yet
            let shouldIgnore = (zone != .permissionPill)
            if win.ignoresMouseEvents != shouldIgnore {
                win.ignoresMouseEvents = shouldIgnore
            }
            setCursorIfNeeded(.arrow)
        } else {
            let shouldIgnore = (zone == .none)
            if win.ignoresMouseEvents != shouldIgnore {
                win.ignoresMouseEvents = shouldIgnore
            }
            if zone == .head || zone == .body {
                setCursorIfNeeded(.openHand)
            } else if zone == .permissionPill {
                setCursorIfNeeded(.pointingHand)
            } else {
                setCursorIfNeeded(.arrow)
            }
        }
    }

    private func setCursorIfNeeded(_ target: ActiveCursor) {
        guard activeCursor != target else { return }
        activeCursor = target
        switch target {
        case .arrow: NSCursor.arrow.set()
        case .openHand: NSCursor.openHand.set()
        case .pointingHand: NSCursor.pointingHand.set()
        case .crosshair: NSCursor.crosshair.set()
        case .closedHand: NSCursor.closedHand.set()
        }
    }

    override func mouseDown(with event: NSEvent) {
        onUserInteraction?()
        guard let win = window else { return }
        let screenPt = NSEvent.mouseLocation
        let showPill = InputEngine.shared.status.needsAttention

        if let pt = canvasPoint(fromScreenPoint: screenPt) {
            if CatSettings.shared.isEditMode {
                let hit = CatRenderer.hitTestEditOverlay(point: pt)
                if hit == 20 {
                    if event.clickCount == 2 {
                        CatSettings.shared.rotationDeg = 0
                        UserDefaults.standard.synchronize()
                        needsDisplay = true
                        return
                    }
                    isRotating = true
                    rotateDragged = false
                    rotateStartDeg = CatSettings.shared.rotationDeg
                    let center = CGPoint(x: win.frame.midX, y: win.frame.midY)
                    rotateStartMouseAngle = atan2(-(screenPt.y - center.y), screenPt.x - center.x)
                    setCursorIfNeeded(.closedHand)
                    return
                }
                if hit >= 0 && hit <= 3 {
                    activeResizeCorner = hit
                    resizeStartFrame = win.frame
                    // Anchor the opposite corner in screen space (AppKit Y is bottom-up)
                    let f = win.frame
                    switch hit {
                    case 0: resizeAnchorScreenPt = CGPoint(x: f.maxX, y: f.minY) // dragging TL -> anchor BR
                    case 1: resizeAnchorScreenPt = CGPoint(x: f.minX, y: f.minY) // dragging TR -> anchor BL
                    case 2: resizeAnchorScreenPt = CGPoint(x: f.maxX, y: f.maxY) // dragging BL -> anchor TR
                    default: resizeAnchorScreenPt = CGPoint(x: f.minX, y: f.maxY) // dragging BR -> anchor TL
                    }
                    return
                }
                if hit < 0 {
                    CatSettings.shared.isEditMode = false
                    needsDisplay = true
                    return
                }
            }

            let catPt = CatRenderer.canvasPointToCatSpace(pt, flipped: CatSettings.shared.flipHorizontal, rotationDeg: CatSettings.shared.rotationDeg)
            let zone = CatRenderer.hitTest(point: catPt, physics: physics, showPermissionPill: showPill)
            if zone == .permissionPill {
                onRequestPermission?()
                return
            }

            // Double-clicking the cat toggles the clean resize & direction edit box!
            if event.clickCount == 2 && (zone == .head || zone == .body || CatSettings.shared.isEditMode) {
                CatSettings.shared.isEditMode.toggle()
                needsDisplay = true
                return
            }
        }

        isDragging = true
        dragStartScreenPt = screenPt
        dragStartWindowOrigin = win.frame.origin
        physics.setDragging(true, dragDx: 0)
        setCursorIfNeeded(.closedHand)
    }

    override func mouseDragged(with event: NSEvent) {
        onUserInteraction?()
        guard let win = window else { return }
        let currentScreenPt = NSEvent.mouseLocation
        if isRotating {
            let center = CGPoint(x: win.frame.midX, y: win.frame.midY)
            let curAngle = atan2(-(currentScreenPt.y - center.y), currentScreenPt.x - center.x)
            var deltaDeg = (curAngle - rotateStartMouseAngle) * 180.0 / .pi
            while deltaDeg > 180 { deltaDeg -= 360 }
            while deltaDeg < -180 { deltaDeg += 360 }
            if abs(deltaDeg) > 2.0 { rotateDragged = true }
            var deg = rotateStartDeg + deltaDeg
            while deg > 180 { deg -= 360 }
            while deg < -180 { deg += 360 }
            // Soft magnetic snap near cardinal angles (0°, ±90°, ±180°)
            for snap in [-180.0, -90.0, 0.0, 90.0, 180.0] as [CGFloat] {
                if abs(deg - snap) < 5.0 { deg = (snap == -180.0) ? 180.0 : snap; break }
            }
            CatSettings.shared.rotationDeg = deg
            needsDisplay = true
            return
        }
        if activeResizeCorner >= 0 {
            let totalCanvasH = CatPhysics.canvasSize.height + CatCanvasView.contentTopInset
            let dx = abs(currentScreenPt.x - resizeAnchorScreenPt.x)
            let dy = abs(currentScreenPt.y - resizeAnchorScreenPt.y)
            let sx = dx / CatPhysics.canvasSize.width
            let sy = dy / totalCanvasH
            let newScale = max(0.45, min(2.50, max(sx, sy)))
            let newW = CatPhysics.canvasSize.width * newScale
            let newH = totalCanvasH * newScale
            let originX = (activeResizeCorner == 0 || activeResizeCorner == 2)
                ? (resizeAnchorScreenPt.x - newW)
                : resizeAnchorScreenPt.x
            let originY = (activeResizeCorner == 2 || activeResizeCorner == 3)
                ? (resizeAnchorScreenPt.y - newH)
                : resizeAnchorScreenPt.y
            CatSettings.shared.scale = newScale
            win.setFrame(CGRect(x: originX, y: originY, width: newW, height: newH), display: true)
            needsDisplay = true
            return
        }
        guard isDragging else { return }
        let prevX = win.frame.origin.x
        let prevY = win.frame.origin.y
        let newOrigin = CGPoint(
            x: dragStartWindowOrigin.x + (currentScreenPt.x - dragStartScreenPt.x),
            y: dragStartWindowOrigin.y + (currentScreenPt.y - dragStartScreenPt.y)
        )
        let rawDx = newOrigin.x - prevX
        let rawDy = -(newOrigin.y - prevY)
        // Transform world drag velocity into cat-local orientation so trailing inertia matches world motion at any angle!
        let rad = -CatSettings.shared.rotationDeg * .pi / 180.0
        let localDx = rawDx * cos(rad) - rawDy * sin(rad)
        let localDy = rawDx * sin(rad) + rawDy * cos(rad)
        physics.setDragging(true, dragDx: CatSettings.shared.flipHorizontal ? -localDx : localDx, dragDy: localDy)
        win.setFrameOrigin(newOrigin)
    }

    override func mouseUp(with event: NSEvent) {
        onUserInteraction?()
        guard let win = window else { return }
        if isRotating {
            isRotating = false
            if !rotateDragged {
                CatSettings.shared.flipHorizontal.toggle()
            }
            UserDefaults.standard.synchronize()
            setCursorIfNeeded(.pointingHand)
            needsDisplay = true
            return
        }
        if activeResizeCorner >= 0 {
            activeResizeCorner = -1
            UserDefaults.standard.set(Double(win.frame.origin.x), forKey: "KuroCat.PosX")
            UserDefaults.standard.set(Double(win.frame.origin.y), forKey: "KuroCat.PosY")
            UserDefaults.standard.synchronize()
            return
        }
        guard isDragging else { return }
        isDragging = false
        physics.setDragging(false)
        setCursorIfNeeded(.openHand)
        // Save position immediately so restarts always preserve exact coordinates
        UserDefaults.standard.set(Double(win.frame.origin.x), forKey: "KuroCat.PosX")
        UserDefaults.standard.set(Double(win.frame.origin.y), forKey: "KuroCat.PosY")
        UserDefaults.standard.synchronize()
    }

    override func rightMouseDown(with event: NSEvent) {
        onUserInteraction?()
        onRightClickMenu?(event)
    }

    override func scrollWheel(with event: NSEvent) {
        let optionHeld = NSEvent.modifierFlags.contains(.option)
        if CatSettings.shared.fullClickThrough && !optionHeld { return }
        if event.momentumPhase != [] { return }
        let raw = event.scrollingDeltaY
        guard abs(raw) > 0.01 else { return }
        let delta: CGFloat = event.hasPreciseScrollingDeltas
            ? max(-0.18, min(0.18, raw * 0.0045))
            : max(-0.20, min(0.20, raw * 0.06))
        guard abs(delta) > 0.0005 else { return }
        onUserInteraction?()
        let newScale = max(0.45, min(2.50, CatSettings.shared.scale * (1.0 + delta)))
        (window as? OverlayWindow)?.applyScale(newScale)
    }

    override func magnify(with event: NSEvent) {
        let optionHeld = NSEvent.modifierFlags.contains(.option)
        if CatSettings.shared.fullClickThrough && !optionHeld { return }
        onUserInteraction?()
        let factor = max(0.5, min(1.5, 1.0 + event.magnification))
        let newScale = max(0.45, min(2.50, CatSettings.shared.scale * factor))
        (window as? OverlayWindow)?.applyScale(newScale)
    }

    // MARK: Drag & drop of .kurocat skins onto the cat

    override func viewDidMoveToWindow() {
        super.viewDidMoveToWindow()
        wantsLayer = true
        layerContentsRedrawPolicy = .onSetNeedsDisplay
        registerForDraggedTypes([.fileURL])
    }

    override func viewDidChangeBackingProperties() {
        super.viewDidChangeBackingProperties()
        cachedForegroundImage = nil
        cachedForegroundPose = nil
        needsDisplay = true
    }

    override func draggingEntered(_ sender: NSDraggingInfo) -> NSDragOperation {
        SkinDropView.fileURLs(from: sender, kurocatOnly: true).isEmpty ? [] : .copy
    }

    override func draggingUpdated(_ sender: NSDraggingInfo) -> NSDragOperation {
        draggingEntered(sender)
    }

    override func performDragOperation(_ sender: NSDraggingInfo) -> Bool {
        let urls = SkinDropView.fileURLs(from: sender, kurocatOnly: true)
        guard !urls.isEmpty else { return false }
        // Import after the drag session ends so an error alert never blocks the drag source.
        DispatchQueue.main.async { [weak self] in self?.onDropSkinFiles?(urls) }
        return true
    }

    // MARK: - Toast API (hourly announcement pill)

    func showToast(text: String) {
        toastText = text
        toastPhase = 0
        toastProgress = 0
        onUserInteraction?()
        needsDisplay = true
    }

    /// Called every frame from the render loop. Returns true while toast is alive.
    @discardableResult
    func tickToast(dt: CGFloat) -> Bool {
        guard toastActive else { return false }
        toastPhase += Double(dt)

        if toastPhase < toastFadeIn {
            toastProgress = CGFloat(toastPhase / toastFadeIn)
        } else if toastPhase < toastFadeIn + toastHold {
            toastProgress = 1.0
        } else if toastPhase < toastFadeIn + toastHold + toastFadeOut {
            let fadeOutElapsed = toastPhase - (toastFadeIn + toastHold)
            toastProgress = CGFloat(1.0 - fadeOutElapsed / toastFadeOut)
        } else {
            toastText = nil
            toastProgress = 0
            toastPhase = 0
            return false
        }
        return true
    }

    private func drawToastPill(in ctx: CGContext, text: String, alpha: CGFloat) {
        guard alpha > 0.01 else { return }

        let attrs: [NSAttributedString.Key: Any] = [
            .font: NSFont.systemFont(ofSize: 18, weight: .semibold),
            .foregroundColor: NSColor.white
        ]
        let attrStr = NSAttributedString(string: text, attributes: attrs)
        let sz = attrStr.size()

        let padX: CGFloat = 20
        let padY: CGFloat = 13
        let pillW = sz.width + padX * 2
        let pillH = sz.height + padY * 2

        // Follow the cat's actual head center so toast stays above the head
        // during scruff-drag, lie-down, sleep, and other pose transitions.
        let skin = CatSettings.shared.activeSkin
        let headCenter = CatRenderer.headPosition(physics, skin: skin)
        let headScale = skin.shape.headScale
        let headTopY = headCenter.y - 38.0 * headScale  // top of cat face ellipse

        let rect = CGRect(
            x: headCenter.x - pillW / 2,
            y: headTopY - pillH - 60,
            width: pillW,
            height: pillH
        )
        let cornerRadius = rect.height / 2

        ctx.saveGState()
        ctx.setAlpha(max(0, min(1, alpha)))
        ctx.setShadow(offset: CGSize(width: 0, height: -2), blur: 8,
                      color: NSColor.black.withAlphaComponent(0.35).cgColor)

        let path = CGPath(roundedRect: rect, cornerWidth: cornerRadius,
                          cornerHeight: cornerRadius, transform: nil)
        ctx.setFillColor(NSColor(calibratedWhite: 0.08, alpha: 0.90).cgColor)
        ctx.addPath(path)
        ctx.fillPath()

        ctx.setShadow(offset: CGSize.zero, blur: 0)
        ctx.setStrokeColor(NSColor(calibratedWhite: 0.45, alpha: 0.55).cgColor)
        ctx.setLineWidth(1.0)
        let inner = rect.insetBy(dx: 0.5, dy: 0.5)
        let innerPath = CGPath(roundedRect: inner, cornerWidth: cornerRadius - 0.5,
                               cornerHeight: cornerRadius - 0.5, transform: nil)
        ctx.addPath(innerPath)
        ctx.strokePath()

        NSGraphicsContext.saveGraphicsState()
        NSGraphicsContext.current = NSGraphicsContext(cgContext: ctx, flipped: true)
        let textY = rect.midY - sz.height / 2
        attrStr.draw(at: CGPoint(x: rect.midX - sz.width / 2, y: textY))
        NSGraphicsContext.restoreGraphicsState()

        ctx.restoreGState()
    }
}

final class OverlayWindow: NSPanel {
    let catView = CatCanvasView()

    init() {
        let scale = CatSettings.shared.scale
        let size = CGSize(
            width: CatPhysics.canvasSize.width * scale,
            height: (CatPhysics.canvasSize.height + CatCanvasView.contentTopInset) * scale
        )
        let origin = OverlayWindow.initialOrigin(for: size)

        super.init(
            contentRect: CGRect(origin: origin, size: size),
            styleMask: [.borderless, .nonactivatingPanel],
            backing: .buffered,
            defer: false
        )

        isOpaque = false
        backgroundColor = .clear
        hasShadow = false
        isMovable = false
        hidesOnDeactivate = false
        becomesKeyOnlyIfNeeded = true
        acceptsMouseMovedEvents = true
        ignoresMouseEvents = true

        applyWindowLevelAndBehavior()
        applyOBSMode()

        catView.frame = CGRect(origin: .zero, size: size)
        catView.autoresizingMask = [.width, .height]
        contentView = catView
    }

    func applyOBSMode() {
        let mode = CatSettings.shared.obsCaptureMode
        let hideDesktop = (mode > 0 && CatSettings.shared.obsHideDesktop)
        title = "KuroCat"
        sharingType = .readOnly
        if mode == 2 {
            isOpaque = true
            backgroundColor = NSColor(calibratedRed: 0.0, green: 1.0, blue: 0.0, alpha: 1.0)
        } else {
            isOpaque = false
            backgroundColor = .clear
        }
        if hideDesktop {
            // Tuck behind desktop/all apps while staying rendered so OBS Window Capture stays live!
            level = NSWindow.Level(rawValue: Int(CGWindowLevelForKey(.desktopIconWindow)) - 1)
            collectionBehavior = [.canJoinAllSpaces, .stationary, .ignoresCycle]
            ignoresMouseEvents = true
        } else {
            applyWindowLevelAndBehavior()
        }
        catView.needsDisplay = true
    }

    func applyWindowLevelAndBehavior() {
        if CatSettings.shared.fullScreenOverlay {
            // popUpMenuWindow (101) stays reliably above WPS/Keynote slideshows, fullscreen apps and Dock
            // while remaining below Spotlight and system Force Quit dialogs.
            level = NSWindow.Level(rawValue: Int(CGWindowLevelForKey(.popUpMenuWindow)))
            collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary, .stationary, .ignoresCycle]
        } else {
            level = .floating
            collectionBehavior = [.canJoinAllSpaces, .stationary, .ignoresCycle]
        }
    }

    func applyScale(_ newScale: CGFloat) {
        CatSettings.shared.scale = newScale
        let oldFrame = frame
        let newSize = CGSize(
            width: CatPhysics.canvasSize.width * newScale,
            height: (CatPhysics.canvasSize.height + CatCanvasView.contentTopInset) * newScale
        )
        // Anchor bottom-center so scaling feels grounded
        let newOrigin = CGPoint(
            x: oldFrame.midX - newSize.width * 0.5,
            y: oldFrame.minY
        )
        setFrame(CGRect(origin: newOrigin, size: newSize), display: true)
        catView.needsDisplay = true
        UserDefaults.standard.set(Double(newOrigin.x), forKey: "KuroCat.PosX")
        UserDefaults.standard.set(Double(newOrigin.y), forKey: "KuroCat.PosY")
    }

    func resetPosition() {
        let origin = OverlayWindow.defaultOrigin(for: frame.size)
        setFrameOrigin(origin)
        catView.needsDisplay = true
        UserDefaults.standard.set(Double(origin.x), forKey: "KuroCat.PosX")
        UserDefaults.standard.set(Double(origin.y), forKey: "KuroCat.PosY")
    }

    /// Frame the physics engine should treat as the cat's actual drawing area
    /// (the window is taller than the cat canvas because of the toast headroom).
    var physicsFrameInScreen: CGRect {
        let scale = frame.width / CatPhysics.canvasSize.width
        var f = frame
        f.size.height -= CatCanvasView.contentTopInset * scale
        return f
    }

    private static func initialOrigin(for size: CGSize) -> CGPoint {
        let d = UserDefaults.standard
        if d.object(forKey: "KuroCat.PosX") != nil && d.object(forKey: "KuroCat.PosY") != nil {
            let x = CGFloat(d.double(forKey: "KuroCat.PosX"))
            let y = CGFloat(d.double(forKey: "KuroCat.PosY"))
            let saved = CGPoint(x: x, y: y)
            // Ensure saved origin is still on a connected display
            let rect = CGRect(origin: saved, size: size)
            if NSScreen.screens.contains(where: { $0.frame.intersects(rect) }) {
                return saved
            }
        }
        return defaultOrigin(for: size)
    }

    private static func defaultOrigin(for size: CGSize) -> CGPoint {
        let screen = NSScreen.main?.visibleFrame ?? CGRect(x: 0, y: 0, width: 1440, height: 900)
        return CGPoint(
            x: screen.maxX - size.width - 28,
            y: screen.minY + 18
        )
    }
}

// MARK: - 「换装间」 skin import window (warm cream card UI shared with Windows/Linux)

private enum DressingPalette {
    static let background = NSColor(calibratedRed: 0.984, green: 0.969, blue: 0.941, alpha: 1)   // #FBF7F0
    static let card = NSColor(calibratedRed: 0.953, green: 0.914, blue: 0.863, alpha: 1)         // #F3E9DC
    static let ink = NSColor(calibratedRed: 0.231, green: 0.184, blue: 0.165, alpha: 1)          // #3B2F2A
    static let brown = NSColor(calibratedRed: 0.353, green: 0.290, blue: 0.251, alpha: 1)        // #5A4A40
    static let muted = NSColor(calibratedRed: 0.659, green: 0.600, blue: 0.553, alpha: 1)        // #A8998D
    static let orange = NSColor(calibratedRed: 0.910, green: 0.588, blue: 0.306, alpha: 1)       // #E8964E
    static let orangeDown = NSColor(calibratedRed: 0.843, green: 0.518, blue: 0.243, alpha: 1)   // #D7843E
    static let dash = NSColor(calibratedRed: 0.890, green: 0.765, blue: 0.627, alpha: 1)         // #E3C3A0
    static let zone = NSColor(calibratedRed: 1.0, green: 0.992, blue: 0.976, alpha: 1)           // #FFFDF9
    static let zoneHover = NSColor(calibratedRed: 1.0, green: 0.945, blue: 0.886, alpha: 1)      // #FFF1E2
    static let ok = NSColor(calibratedRed: 0.243, green: 0.557, blue: 0.353, alpha: 1)           // #3E8E5A
    static let err = NSColor(calibratedRed: 0.761, green: 0.286, blue: 0.231, alpha: 1)          // #C2493B
    static let note = NSColor(calibratedRed: 0.541, green: 0.482, blue: 0.439, alpha: 1)         // #8A7B70
    static let footer = NSColor(calibratedRed: 0.702, green: 0.651, blue: 0.608, alpha: 1)       // #B3A69B
}

private final class FlippedView: NSView {
    override var isFlipped: Bool { true }
}

/// Live preview of the skin being worn, drawn by the real renderer into a rounded card.
private final class SkinPreviewView: NSView {
    private let pose = CatPhysics()
    override var isFlipped: Bool { true }
    override func draw(_ dirtyRect: NSRect) {
        guard let ctx = NSGraphicsContext.current?.cgContext else { return }
        let card = NSBezierPath(roundedRect: bounds.insetBy(dx: 0.5, dy: 0.5), xRadius: 18, yRadius: 18)
        DressingPalette.card.setFill()
        card.fill()
        card.addClip()
        ctx.saveGState()
        ctx.scaleBy(x: bounds.width / 290, y: bounds.height / 260)
        ctx.translateBy(x: -40, y: -36)
        CatRenderer.draw(in: ctx, physics: pose, showPermissionPill: false, isPreview: true)
        ctx.restoreGState()
    }
}

/// Dashed drop zone of the 「换装间」 window; click it to pick a file.
final class SkinDropView: NSView {
    var onFiles: (([URL]) -> Void)?
    var onClick: (() -> Void)?
    private var highlighted = false { didSet { needsDisplay = true } }
    override var isFlipped: Bool { true }

    override init(frame frameRect: NSRect) {
        super.init(frame: frameRect)
        registerForDraggedTypes([.fileURL])
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    static func fileURLs(from info: NSDraggingInfo, kurocatOnly: Bool) -> [URL] {
        let objects = info.draggingPasteboard.readObjects(forClasses: [NSURL.self], options: [.urlReadingFileURLsOnly: true])
        let urls = (objects as? [URL]) ?? []
        if !kurocatOnly { return urls }
        return urls.filter { url in
            let ext = url.pathExtension.lowercased()
            return ext == "kurocat" || ext == "kurokb" || (ext == "json" && KeyboardSkinSpec.looksLikeKeyboardJSON(url))
        }
    }

    override func draggingEntered(_ sender: NSDraggingInfo) -> NSDragOperation {
        let ok = !SkinDropView.fileURLs(from: sender, kurocatOnly: false).isEmpty
        highlighted = ok
        return ok ? .copy : []
    }

    override func draggingExited(_ sender: NSDraggingInfo?) {
        highlighted = false
    }

    override func performDragOperation(_ sender: NSDraggingInfo) -> Bool {
        highlighted = false
        let urls = SkinDropView.fileURLs(from: sender, kurocatOnly: false)
        guard !urls.isEmpty else { return false }
        DispatchQueue.main.async { [weak self] in self?.onFiles?(urls) }
        return true
    }

    override func mouseDown(with event: NSEvent) {
        // Claim the click so the matching mouseUp comes back to this view.
    }

    override func mouseUp(with event: NSEvent) {
        if bounds.contains(convert(event.locationInWindow, from: nil)) { onClick?() }
    }

    override func resetCursorRects() {
        addCursorRect(bounds, cursor: .pointingHand)
    }

    override func draw(_ dirtyRect: NSRect) {
        let box = NSBezierPath(roundedRect: bounds.insetBy(dx: 1.5, dy: 1.5), xRadius: 16, yRadius: 16)
        (highlighted ? DressingPalette.zoneHover : DressingPalette.zone).setFill()
        box.fill()
        box.lineWidth = highlighted ? 2.4 : 2
        if !highlighted {
            let dash: [CGFloat] = [7, 5]
            box.setLineDash(dash, count: dash.count, phase: 0)
        }
        (highlighted ? DressingPalette.orange : DressingPalette.dash).setStroke()
        box.stroke()

        // Paw print
        let k: CGFloat = highlighted ? 1.15 : 1
        let c = CGPoint(x: bounds.midX, y: bounds.midY - 26)
        DressingPalette.orange.withAlphaComponent(highlighted ? 1 : 0.85).setFill()
        NSBezierPath(ovalIn: NSRect(x: c.x - 9.5 * k, y: c.y + (6 - 7.8) * k, width: 19 * k, height: 15.6 * k)).fill()
        for (tx, ty) in [(-10.5, -5.5), (-3.8, -11.5), (3.8, -11.5), (10.5, -5.5)] as [(CGFloat, CGFloat)] {
            NSBezierPath(ovalIn: NSRect(x: c.x + (tx - 3.7) * k, y: c.y + (ty - 4.4) * k, width: 7.4 * k, height: 8.8 * k)).fill()
        }

        func centered(_ string: String, font: NSFont, color: NSColor, y: CGFloat) {
            let attrs: [NSAttributedString.Key: Any] = [.font: font, .foregroundColor: color]
            let size = (string as NSString).size(withAttributes: attrs)
            (string as NSString).draw(at: NSPoint(x: bounds.midX - size.width / 2, y: y), withAttributes: attrs)
        }
        centered(highlighted ? loc("松手就换上啦", "Drop to wear it", "Solte para vestir")
                             : loc("把 .kurocat 皮肤拖到这里", "Drag a .kurocat skin here", "Arraste uma skin .kurocat para cá"), font: .systemFont(ofSize: 15, weight: .bold),
                 color: DressingPalette.brown, y: bounds.midY + 2)
        centered(loc("也可以直接拖到桌面上的小猫身上 · 点这里选择文件",
                     "Or drop it on the cat on your desktop · Click to choose a file",
                     "Ou solte em cima do gato na tela · Clique para escolher um arquivo"), font: .systemFont(ofSize: 11.5),
                 color: DressingPalette.note, y: bounds.midY + 28)
    }
}

/// Orange pill button.
private final class PillButton: NSButton {
    override func draw(_ dirtyRect: NSRect) {
        let pill = NSBezierPath(roundedRect: bounds.insetBy(dx: 1, dy: 1), xRadius: (bounds.height - 2) / 2, yRadius: (bounds.height - 2) / 2)
        (isHighlighted ? DressingPalette.orangeDown : DressingPalette.orange).setFill()
        pill.fill()
        let attrs: [NSAttributedString.Key: Any] = [.font: NSFont.systemFont(ofSize: 14.5, weight: .bold), .foregroundColor: NSColor.white]
        let size = (title as NSString).size(withAttributes: attrs)
        (title as NSString).draw(at: NSPoint(x: bounds.midX - size.width / 2, y: bounds.midY - size.height / 2), withAttributes: attrs)
    }
    override func resetCursorRects() {
        addCursorRect(bounds, cursor: .pointingHand)
    }
}

/// 「换装间」: import `.kurocat` skins with drag & drop or a file picker.
final class DressingRoomWindowController: NSWindowController {
    var onImport: ((URL) -> Void)?
    private let preview = SkinPreviewView(frame: NSRect(x: 22, y: 22, width: 150, height: 134))
    private let nameLabel = NSTextField(labelWithString: "")
    private let authorLabel = NSTextField(labelWithString: "")
    private let statusLabel = NSTextField(wrappingLabelWithString: "")
    private let footerLabel = NSTextField(wrappingLabelWithString: "")

    init() {
        let window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 444, height: 472),
                              styleMask: [.titled, .closable], backing: .buffered, defer: false)
        window.title = loc("换装间", "Dressing Room", "Provador")
        window.isReleasedWhenClosed = false
        window.level = .floating
        window.titlebarAppearsTransparent = true
        window.backgroundColor = DressingPalette.background
        window.appearance = NSAppearance(named: .aqua)
        super.init(window: window)

        let content = FlippedView(frame: NSRect(x: 0, y: 0, width: 444, height: 472))
        func label(_ text: String, _ font: NSFont, _ color: NSColor, _ frame: NSRect, wraps: Bool = false) -> NSTextField {
            let field = wraps ? NSTextField(wrappingLabelWithString: text) : NSTextField(labelWithString: text)
            field.font = font
            field.textColor = color
            field.frame = frame
            return field
        }
        content.addSubview(preview)
        content.addSubview(label(loc("换装间", "Dressing Room", "Provador"), .systemFont(ofSize: 21, weight: .heavy), DressingPalette.ink, NSRect(x: 190, y: 22, width: 240, height: 30)))
        content.addSubview(label(loc("正在穿", "Wearing", "Vestindo agora"), .systemFont(ofSize: 11.5), DressingPalette.muted, NSRect(x: 191, y: 58, width: 230, height: 16)))
        nameLabel.font = .systemFont(ofSize: 15, weight: .bold)
        nameLabel.textColor = DressingPalette.brown
        nameLabel.lineBreakMode = .byTruncatingTail
        nameLabel.frame = NSRect(x: 190, y: 74, width: 236, height: 22)
        authorLabel.font = .systemFont(ofSize: 11.5)
        authorLabel.textColor = DressingPalette.muted
        authorLabel.lineBreakMode = .byTruncatingTail
        authorLabel.frame = NSRect(x: 191, y: 97, width: 232, height: 16)
        content.addSubview(nameLabel)
        content.addSubview(authorLabel)
        let hint = label(loc("导入皮肤工坊做好的 .kurocat 文件，\n小猫会立刻换上新毛色。",
                             "Import a .kurocat file made in the Skin Workshop\nand the cat changes its coat right away.",
                             "Importe um arquivo .kurocat feito na Oficina de skins\ne o gato troca de pelagem na hora."), .systemFont(ofSize: 11.5), DressingPalette.muted,
                         NSRect(x: 191, y: 120, width: 236, height: 36), wraps: true)
        content.addSubview(hint)

        let drop = SkinDropView(frame: NSRect(x: 22, y: 172, width: 400, height: 150))
        drop.onFiles = { [weak self] urls in urls.forEach { self?.onImport?($0) } }
        drop.onClick = { [weak self] in self?.chooseFile() }
        content.addSubview(drop)

        let button = PillButton(title: loc("选择文件…", "Choose File…", "Escolher arquivo…"), target: self, action: #selector(chooseFile))
        button.isBordered = false
        button.frame = NSRect(x: 147, y: 338, width: 150, height: 40)
        content.addSubview(button)

        statusLabel.alignment = .center
        statusLabel.font = .systemFont(ofSize: 12.5)
        statusLabel.frame = NSRect(x: 22, y: 390, width: 400, height: 40)
        content.addSubview(statusLabel)

        footerLabel.font = .systemFont(ofSize: 10.5)
        footerLabel.textColor = DressingPalette.footer
        footerLabel.frame = NSRect(x: 22, y: 434, width: 400, height: 34)
        footerLabel.alignment = .center
        content.addSubview(footerLabel)

        window.contentView = content
        window.center()
        setStatus(loc("支持拖入多个文件；坏掉的文件会告诉你哪里不对。",
                      "You can drop several files; broken ones will tell you what's wrong.",
                      "Pode soltar vários arquivos; se algum estiver com problema, você verá o motivo."), kind: 0)
        refresh()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    /// kind: 0 neutral, 1 success, 2 error
    func setStatus(_ text: String, kind: Int) {
        statusLabel.stringValue = text
        statusLabel.textColor = kind == 1 ? DressingPalette.ok : (kind == 2 ? DressingPalette.err : DressingPalette.note)
        statusLabel.font = .systemFont(ofSize: 12.5, weight: kind == 0 ? .regular : .semibold)
    }

    /// Re-reads the worn skin (name, author, live preview).
    func refresh() {
        let skin = CatSettings.shared.activeSkin
        nameLabel.stringValue = skin.displayName
        authorLabel.stringValue = skin.author.isEmpty ? "" : loc("作者 \(skin.author)", "By \(skin.author)", "Por \(skin.author)")
        let footerPath = (SkinLibrary.directory.path as NSString).abbreviatingWithTildeInPath
        footerLabel.stringValue = loc("皮肤独立保存在 \(footerPath)（软件更新会自动保留）  ·  右键小猫 → 猫咪皮肤 可切换或备份",
                                      "Skins are saved in \(footerPath) (kept across updates)  ·  Right-click the cat → Cat Skin to switch",
                                      "As skins ficam em \(footerPath) (mantidas nas atualizações)  ·  Clique direito no gato → Skin do gato para trocar")
        preview.needsDisplay = true
    }

    var isVisible: Bool { window?.isVisible ?? false }

    func present() {
        refresh()
        NSApp.activate(ignoringOtherApps: true)
        showWindow(nil)
        window?.makeKeyAndOrderFront(nil)
    }

    @objc private func chooseFile() {
        guard let window else { return }
        let panel = NSOpenPanel()
        panel.title = loc("选择 .kurocat 皮肤文件", "Choose a .kurocat Skin File", "Escolher arquivo de skin .kurocat")
        panel.prompt = loc("导入", "Import", "Importar")
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = true
        panel.allowedContentTypes = [UTType(filenameExtension: "kurocat") ?? .data]
        panel.beginSheetModal(for: window) { [weak self] response in
            guard response == .OK else { return }
            for url in panel.urls { self?.onImport?(url) }
        }
    }
}

// MARK: - Unified Native Settings Window (System Light/Dark Adaptive)

private final class SettingsCardView: NSView {
    override var isFlipped: Bool { true }
    override func draw(_ dirtyRect: NSRect) {
        let rect = bounds.insetBy(dx: 0.5, dy: 0.5)
        let path = NSBezierPath(roundedRect: rect, xRadius: 12, yRadius: 12)
        NSColor.controlBackgroundColor.setFill()
        path.fill()
        NSColor.separatorColor.withAlphaComponent(0.55).setStroke()
        path.lineWidth = 1.0
        path.stroke()
    }
}

/// Consolidated native macOS Settings window grouping Appearance, Interaction, and Window/System controls.
final class SettingsWindowController: NSWindowController {
    var onSettingsChanged: (() -> Void)?
    var onScaleChanged: ((CGFloat) -> Void)?
    var onResetPoseAndPosition: (() -> Void)?
    var onOpenDressingRoom: (() -> Void)?
    var onSkinsDirectoryChanged: (() -> Void)?
    var onRenameSkin: ((String) -> Void)?
    var onDeleteSkin: ((String) -> Void)?
    var onImportKeyboardSkin: (() -> Void)?
    var onApplyWindowLevel: (() -> Void)?
    var onApplyOBSMode: (() -> Void)?
    var onAutoHideChanged: (() -> Void)?
    var getAutoStart: (() -> Bool)?
    var setAutoStart: ((Bool) -> Void)?
    var onLanguageChanged: (() -> Void)?

    private var skinPopup: NSPopUpButton!
    private var skinsDirLabel: NSTextField!
    private var resetSkinsDirBtn: NSButton!
    private var kbPopup: NSPopUpButton!
    private var eyePopup: NSPopUpButton!
    private var tabletPopup: NSPopUpButton!
    private var typingLimbPopup: NSPopUpButton!
    private var obsPopup: NSPopUpButton!
    private var langSeg: NSSegmentedControl!

    private var whiteOutlineSwitch: NSSwitch!
    private var flipSwitch: NSSwitch!
    private var angerSwitch: NSSwitch!
    private var dragAnimSwitch: NSSwitch!
    private var badgesSwitch: NSSwitch!
    private var mouseBadgesSwitch: NSSwitch!
    private var legendsSwitch: NSSwitch!
    private var marqueeSwitch: NSSwitch!
    private var levelKeyboardSwitch: NSSwitch!
    private var fullscreenOverlaySwitch: NSSwitch!
    private var clickThroughSwitch: NSSwitch!
    private var autoHideSwitch: NSSwitch!
    private var obsHideSwitch: NSSwitch!
    private var autoStartSwitch: NSSwitch!
    private var hourlySwitch: NSSwitch!

    private var rotSlider: NSSlider!
    private var rotValLabel: NSTextField!
    private var scaleSlider: NSSlider!
    private var scaleValLabel: NSTextField!

    private var skinIds: [String] = []
    private var kbIds: [String] = []

    init() {
        let win = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 492, height: 610),
            styleMask: [.titled, .closable],
            backing: .buffered,
            defer: false
        )
        win.isReleasedWhenClosed = false
        win.level = .floating
        win.titlebarAppearsTransparent = false
        win.backgroundColor = .windowBackgroundColor
        super.init(window: win)
        rebuildUI()
        win.center()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func tr(_ zh: String, _ en: String, _ pt: String) -> String {
        loc(zh, en, pt)
    }

    func present() {
        rebuildUI()
        NSApp.activate(ignoringOtherApps: true)
        showWindow(nil)
        window?.makeKeyAndOrderFront(nil)
    }

    func refresh() {
        guard window?.isVisible == true else { return }
        rebuildUI()
    }

    private func rebuildUI() {
        guard let win = window else { return }
        let s = CatSettings.shared
        win.title = tr("KuroCat 设置", "KuroCat Settings", "Configurações do KuroCat")

        let scroll = NSScrollView(frame: NSRect(x: 0, y: 0, width: 492, height: 610))
        scroll.hasVerticalScroller = true
        scroll.drawsBackground = false
        scroll.borderType = .noBorder

        let doc = FlippedView(frame: NSRect(x: 0, y: 0, width: 492, height: 760))
        let padX: CGFloat = 18
        let cardW: CGFloat = 456
        var y: CGFloat = 14

        func addSectionHeader(_ text: String) {
            let lbl = NSTextField(labelWithString: text)
            lbl.font = .systemFont(ofSize: 12, weight: .semibold)
            lbl.textColor = .secondaryLabelColor
            lbl.frame = NSRect(x: padX + 6, y: y, width: cardW - 12, height: 18)
            doc.addSubview(lbl)
            y += 22
        }

        func addCard(rows: Int, rowH: CGFloat = 36, builder: (NSView, CGFloat) -> Void) {
            let h = CGFloat(rows) * rowH + 8
            let card = SettingsCardView(frame: NSRect(x: padX, y: y, width: cardW, height: h))
            builder(card, rowH)
            doc.addSubview(card)
            y += h + 14
        }

        func addRowDivider(_ card: NSView, yPos: CGFloat) {
            let div = NSBox(frame: NSRect(x: 14, y: yPos, width: cardW - 28, height: 1))
            div.boxType = .separator
            card.addSubview(div)
        }

        func makeLabel(_ text: String, yPos: CGFloat, width: CGFloat = 240) -> NSTextField {
            let lbl = NSTextField(labelWithString: text)
            lbl.font = .systemFont(ofSize: 13)
            lbl.textColor = .labelColor
            lbl.frame = NSRect(x: 14, y: yPos + 8, width: width, height: 20)
            return lbl
        }

        func makeSwitch(_ on: Bool, action: Selector, yPos: CGFloat) -> NSSwitch {
            let sw = NSSwitch()
            sw.state = on ? .on : .off
            sw.target = self
            sw.action = action
            sw.controlSize = .small
            sw.sizeToFit()
            sw.frame = NSRect(x: cardW - 14 - sw.frame.width, y: yPos + (36 - sw.frame.height) * 0.5, width: sw.frame.width, height: sw.frame.height)
            return sw
        }

        // MARK: Card 1 — Appearance & Orientation
        addSectionHeader(tr("外观与方向", "Appearance & Orientation", "Aparência e orientação"))
        addCard(rows: 8) { card, rh in
            // Row 0: Cat Skin + Dressing Room + Rename/Delete buttons (when a custom skin is active)
            let hasCustomActive = !s.customSkin.isEmpty && SkinLibrary.shared.skin(id: s.customSkin) != nil
            card.addSubview(makeLabel(tr("猫咪皮肤", "Cat Skin", "Skin do gato"), yPos: 4, width: 100))
            skinPopup = NSPopUpButton(frame: NSRect(x: 112, y: 9, width: hasCustomActive ? 148 : 240, height: 24), pullsDown: false)
            skinIds = []
            for coat in CatCoatTheme.allCases {
                skinPopup.addItem(withTitle: coat.displayName)
                skinIds.append("coat:\(coat.rawValue)")
            }
            for sk in SkinLibrary.shared.skins {
                skinPopup.addItem(withTitle: SkinLibrary.shared.displayName(for: sk))
                skinIds.append("custom:\(sk.id)")
            }
            if !s.customSkin.isEmpty, let idx = skinIds.firstIndex(of: "custom:\(s.customSkin)") {
                skinPopup.selectItem(at: idx)
            } else if let idx = skinIds.firstIndex(of: "coat:\(s.coatTheme.rawValue)") {
                skinPopup.selectItem(at: idx)
            }
            skinPopup.target = self
            skinPopup.action = #selector(onSkinSelected)
            card.addSubview(skinPopup)

            if hasCustomActive {
                let dressBtn = NSButton(title: tr("换装间…", "Skins…", "Skins…"), target: self, action: #selector(onTapDressingRoom))
                dressBtn.bezelStyle = .rounded
                dressBtn.controlSize = .small
                dressBtn.frame = NSRect(x: 264, y: 10, width: 64, height: 22)
                card.addSubview(dressBtn)

                let renameBtn = NSButton(title: tr("✏️改名", "✏️", "✏️"), target: self, action: #selector(onTapRenameSkin))
                renameBtn.bezelStyle = .rounded
                renameBtn.controlSize = .small
                renameBtn.frame = NSRect(x: 332, y: 10, width: 52, height: 22)
                card.addSubview(renameBtn)

                let delBtn = NSButton(title: tr("🗑删除", "🗑Del", "🗑"), target: self, action: #selector(onTapDeleteSkin))
                delBtn.bezelStyle = .rounded
                delBtn.controlSize = .small
                delBtn.contentTintColor = .systemRed
                if #available(macOS 11.0, *) { delBtn.hasDestructiveAction = true }
                delBtn.frame = NSRect(x: 388, y: 10, width: 56, height: 22)
                card.addSubview(delBtn)
            } else {
                let dressBtn = NSButton(title: tr("换装间…", "Skins…", "Skins…"), target: self, action: #selector(onTapDressingRoom))
                dressBtn.bezelStyle = .rounded
                dressBtn.controlSize = .small
                dressBtn.frame = NSRect(x: 358, y: 10, width: 84, height: 22)
                card.addSubview(dressBtn)
            }
            addRowDivider(card, yPos: 4 + rh)

            // Row 1: Skins Folder path + Choose / Open / Reset
            let r1 = 4 + rh
            card.addSubview(makeLabel(tr("皮肤目录", "Skins Dir", "Pasta"), yPos: r1, width: 66))
            let dirDisplay = (SkinLibrary.directory.path as NSString).abbreviatingWithTildeInPath
            skinsDirLabel = NSTextField(labelWithString: dirDisplay)
            skinsDirLabel.font = .systemFont(ofSize: 11.5)
            skinsDirLabel.textColor = .secondaryLabelColor
            skinsDirLabel.lineBreakMode = .byTruncatingMiddle
            skinsDirLabel.toolTip = SkinLibrary.directory.path
            skinsDirLabel.frame = NSRect(x: 84, y: r1 + 10, width: 138, height: 17)
            card.addSubview(skinsDirLabel)

            let chooseDirBtn = NSButton(title: tr("选择文件夹…", "Choose…", "Escolher…"), target: self, action: #selector(onTapChooseSkinsDir))
            chooseDirBtn.bezelStyle = .rounded
            chooseDirBtn.controlSize = .small
            chooseDirBtn.frame = NSRect(x: 226, y: r1 + 7, width: 96, height: 22)
            card.addSubview(chooseDirBtn)

            let openDirBtn = NSButton(title: tr("打开", "Open", "Abrir"), target: self, action: #selector(onTapOpenSkinsDir))
            openDirBtn.bezelStyle = .rounded
            openDirBtn.controlSize = .small
            openDirBtn.frame = NSRect(x: 326, y: r1 + 7, width: 46, height: 22)
            card.addSubview(openDirBtn)

            resetSkinsDirBtn = NSButton(title: tr("恢复默认", "Reset", "Redefinir"), target: self, action: #selector(onTapResetSkinsDir))
            resetSkinsDirBtn.bezelStyle = .rounded
            resetSkinsDirBtn.controlSize = .small
            resetSkinsDirBtn.isEnabled = SkinLibrary.isUsingCustomDirectory
            resetSkinsDirBtn.frame = NSRect(x: 376, y: r1 + 7, width: 66, height: 22)
            card.addSubview(resetSkinsDirBtn)
            addRowDivider(card, yPos: r1 + rh)

            // Row 2: Keyboard Theme + Import button
            let r2 = 4 + rh * 2
            card.addSubview(makeLabel(tr("键盘配色", "Keyboard Theme", "Tema do teclado"), yPos: r2, width: 110))
            kbPopup = NSPopUpButton(frame: NSRect(x: 130, y: r2 + 5, width: 222, height: 24), pullsDown: false)
            kbIds = []
            KeyboardSkinLibrary.shared.reload()
            for kb in KeyboardTheme.allCases {
                kbPopup.addItem(withTitle: kb.displayName)
                kbIds.append("theme:\(kb.rawValue)")
            }
            for kb in KeyboardSkinLibrary.shared.skins {
                kbPopup.addItem(withTitle: kb.name)
                kbIds.append("custom:\(kb.id)")
            }
            if !s.customKeyboard.isEmpty, let idx = kbIds.firstIndex(of: "custom:\(s.customKeyboard)") {
                kbPopup.selectItem(at: idx)
            } else if let idx = kbIds.firstIndex(of: "theme:\(s.keyboardTheme.rawValue)") {
                kbPopup.selectItem(at: idx)
            }
            kbPopup.target = self
            kbPopup.action = #selector(onKBSelected)
            card.addSubview(kbPopup)

            let kbBtn = NSButton(title: tr("导入…", "Import…", "Importar…"), target: self, action: #selector(onTapImportKB))
            kbBtn.bezelStyle = .rounded
            kbBtn.controlSize = .small
            kbBtn.frame = NSRect(x: 358, y: r2 + 6, width: 84, height: 22)
            card.addSubview(kbBtn)
            addRowDivider(card, yPos: r2 + rh)

            // Row 3: Eye Color
            let r3 = 4 + rh * 3
            card.addSubview(makeLabel(tr("瞳色切换", "Eye Color", "Cor dos olhos"), yPos: r3, width: 110))
            eyePopup = NSPopUpButton(frame: NSRect(x: 130, y: r3 + 5, width: 312, height: 24), pullsDown: false)
            for eye in EyeColorTheme.allCases {
                eyePopup.addItem(withTitle: eye.displayName)
            }
            if let idx = EyeColorTheme.allCases.firstIndex(of: s.eyeTheme) { eyePopup.selectItem(at: idx) }
            eyePopup.target = self
            eyePopup.action = #selector(onEyeSelected)
            card.addSubview(eyePopup)
            addRowDivider(card, yPos: r3 + rh)

            // Row 4: White Outline toggle
            let r4 = 4 + rh * 4
            card.addSubview(makeLabel(tr("小猫白色外描边（贴纸风）", "White Sticker Outline", "Contorno branco (estilo adesivo)"), yPos: r4, width: 320))
            whiteOutlineSwitch = makeSwitch(s.whiteOutline, action: #selector(onToggleWhiteOutline), yPos: r4)
            card.addSubview(whiteOutlineSwitch)
            addRowDivider(card, yPos: r4 + rh)

            // Row 5: Horizontal Flip toggle
            let r5 = 4 + rh * 5
            card.addSubview(makeLabel(tr("水平镜像翻转（左右朝向）", "Flip Horizontal", "Espelhar horizontalmente"), yPos: r5, width: 320))
            flipSwitch = makeSwitch(s.flipHorizontal, action: #selector(onToggleFlip), yPos: r5)
            card.addSubview(flipSwitch)
            addRowDivider(card, yPos: r5 + rh)

            // Row 6: Rotation Angle slider + Reset 0°
            let r6 = 4 + rh * 6
            card.addSubview(makeLabel(tr("旋转方向", "Rotation", "Rotação"), yPos: r6, width: 96))
            rotSlider = NSSlider(value: Double(s.rotationDeg), minValue: -180, maxValue: 180, target: self, action: #selector(onRotationChanged))
            rotSlider.frame = NSRect(x: 112, y: r6 + 8, width: 204, height: 20)
            card.addSubview(rotSlider)
            rotValLabel = NSTextField(labelWithString: "\(Int(s.rotationDeg.rounded()))°")
            rotValLabel.font = .monospacedDigitSystemFont(ofSize: 12, weight: .medium)
            rotValLabel.textColor = .secondaryLabelColor
            rotValLabel.alignment = .right
            rotValLabel.frame = NSRect(x: 320, y: r6 + 9, width: 42, height: 18)
            card.addSubview(rotValLabel)
            let zeroBtn = NSButton(title: tr("回正", "0°", "0°"), target: self, action: #selector(onResetRotation))
            zeroBtn.bezelStyle = .rounded
            zeroBtn.controlSize = .small
            zeroBtn.frame = NSRect(x: 368, y: r6 + 7, width: 74, height: 22)
            card.addSubview(zeroBtn)
            addRowDivider(card, yPos: r6 + rh)

            // Row 7: Scale slider + Reset Pose & Position
            let r7 = 4 + rh * 7
            card.addSubview(makeLabel(tr("桌宠大小", "Scale", "Tamanho"), yPos: r7, width: 96))
            scaleSlider = NSSlider(value: Double(s.scale), minValue: 0.45, maxValue: 2.0, target: self, action: #selector(onScaleSliderChanged))
            scaleSlider.frame = NSRect(x: 112, y: r7 + 8, width: 204, height: 20)
            card.addSubview(scaleSlider)
            scaleValLabel = NSTextField(labelWithString: "\(Int((s.scale * 100).rounded()))%")
            scaleValLabel.font = .monospacedDigitSystemFont(ofSize: 12, weight: .medium)
            scaleValLabel.textColor = .secondaryLabelColor
            scaleValLabel.alignment = .right
            scaleValLabel.frame = NSRect(x: 320, y: r7 + 9, width: 42, height: 18)
            card.addSubview(scaleValLabel)
            let resetAllBtn = NSButton(title: tr("重置位置", "Reset", "Redefinir"), target: self, action: #selector(onTapResetPose))
            resetAllBtn.bezelStyle = .rounded
            resetAllBtn.controlSize = .small
            resetAllBtn.frame = NSRect(x: 368, y: r7 + 7, width: 74, height: 22)
            card.addSubview(resetAllBtn)
        }

        // MARK: Card 2 — Interaction & Effects
        addSectionHeader(tr("互动与特效", "Interaction & Effects", "Interação e efeitos"))
        addCard(rows: 9) { card, rh in
            // Row 0: Scruff-drag animation toggle
            card.addSubview(makeLabel(tr("拎起拖拽动画（拖动时切换拎起姿态）", "Scruff-Drag Lift Animation", "Animação de erguer ao arrastar"), yPos: 4, width: 320))
            dragAnimSwitch = makeSwitch(s.enableDragAnimation, action: #selector(onToggleDragAnim), yPos: 4)
            card.addSubview(dragAnimSwitch)
            addRowDivider(card, yPos: 4 + rh)

            // Row 1: Fast-typing anger toggle
            let r1 = 4 + rh
            card.addSubview(makeLabel(tr("打字过快时生气表情", "Angry Expression on Fast Typing", "Cara brava ao digitar rápido demais"), yPos: r1, width: 320))
            angerSwitch = makeSwitch(s.enableAnger, action: #selector(onToggleAnger), yPos: r1)
            card.addSubview(angerSwitch)
            addRowDivider(card, yPos: r1 + rh)

            // Row 2: Key badges
            let r2 = 4 + rh * 2
            card.addSubview(makeLabel(tr("显示按键气泡 ([S] [W] [⇧])", "Show Keyboard Key Badges", "Mostrar balões das teclas ([S] [W] [⇧])"), yPos: r2, width: 320))
            badgesSwitch = makeSwitch(s.showKeyBadges, action: #selector(onToggleBadges), yPos: r2)
            card.addSubview(badgesSwitch)
            addRowDivider(card, yPos: r2 + rh)

            // Row 3: Mouse click badges
            let r3 = 4 + rh * 3
            card.addSubview(makeLabel(tr("显示鼠标点击 (左键 / 右键)", "Show Mouse Click Badges", "Mostrar cliques do mouse (esq. / dir.)"), yPos: r3, width: 320))
            mouseBadgesSwitch = makeSwitch(s.showMouseBadges, action: #selector(onToggleMouseBadges), yPos: r3)
            card.addSubview(mouseBadgesSwitch)
            addRowDivider(card, yPos: r3 + rh)

            // Row 4: Multi-key typing limb mode (3rd hind foot / tail / two paws only)
            let r4 = 4 + rh * 4
            card.addSubview(makeLabel(tr("多键打字方式", "Multi-Key Typing", "Várias teclas"), yPos: r4, width: 130))
            typingLimbPopup = NSPopUpButton(frame: NSRect(x: 150, y: r4 + 5, width: 292, height: 24), pullsDown: false)
            typingLimbPopup.addItems(withTitles: [
                tr("后脚 + 尾巴（3键后脚 / 4键尾巴）", "Hind Foot + Tail (3-Key / 4-Key)", "Pata traseira + cauda (3 / 4 teclas)"),
                tr("用第三只脚打字（仅后脚）", "3rd Hind Foot Only", "Só a pata traseira"),
                tr("用尾巴打字（仅尾巴）", "Tail Typing Only", "Só a cauda"),
                tr("只用两只爪爪打字", "Two Front Paws Only", "Só as duas patas da frente")
            ])
            let limbIdx: Int = s.footEasterEgg
                ? (s.tailEasterEgg ? 0 : 1)
                : (s.tailEasterEgg ? 2 : 3)
            typingLimbPopup.selectItem(at: limbIdx)
            typingLimbPopup.target = self
            typingLimbPopup.action = #selector(onTypingLimbSelected)
            card.addSubview(typingLimbPopup)
            addRowDivider(card, yPos: r4 + rh)

            // Row 5: Key legends
            let r5 = 4 + rh * 5
            card.addSubview(makeLabel(tr("显示键盘字符", "Show Keycap Legends", "Mostrar letras nas teclas"), yPos: r5, width: 320))
            legendsSwitch = makeSwitch(s.showKeyLegends, action: #selector(onToggleLegends), yPos: r5)
            card.addSubview(legendsSwitch)
            addRowDivider(card, yPos: r5 + rh)

            // Row 6: Keyboard marquee
            let r6 = 4 + rh * 6
            card.addSubview(makeLabel(tr("键盘跑马灯流光", "Keyboard RGB Marquee", "Luz RGB animada no teclado"), yPos: r6, width: 320))
            marqueeSwitch = makeSwitch(s.keyboardMarquee, action: #selector(onToggleMarquee), yPos: r6)
            card.addSubview(marqueeSwitch)
            addRowDivider(card, yPos: r6 + rh)

            // Row 7: Level keyboard (parallel to screen)
            let r7 = 4 + rh * 7
            card.addSubview(makeLabel(tr("键盘水平摆放（平行屏幕）", "Level Keyboard (Parallel to Screen)", "Teclado nivelado (paralelo à tela)"), yPos: r7, width: 320))
            levelKeyboardSwitch = makeSwitch(s.levelKeyboard, action: #selector(onToggleLevelKeyboard), yPos: r7)
            card.addSubview(levelKeyboardSwitch)
            addRowDivider(card, yPos: r7 + rh)

            // Row 8: Drawing Tablet & Stylus
            let r8 = 4 + rh * 8
            card.addSubview(makeLabel(tr("数位板与触控笔", "Tablet & Stylus", "Mesa e caneta"), yPos: r8, width: 130))
            tabletPopup = NSPopUpButton(frame: NSRect(x: 150, y: r8 + 5, width: 292, height: 24), pullsDown: false)
            tabletPopup.addItems(withTitles: [
                tr("自动检测（连接数位板时切换）", "Auto-Detect When Connected", "Detectar ao conectar mesa digitalizadora"),
                tr("始终显示数位板与触控笔", "Always Show Tablet & Stylus", "Sempre mostrar mesa e caneta"),
                tr("始终显示鼠标", "Always Show Mouse", "Sempre mostrar mouse")
            ])
            tabletPopup.selectItem(at: max(0, min(2, s.tabletMode)))
            tabletPopup.target = self
            tabletPopup.action = #selector(onTabletSelected)
            card.addSubview(tabletPopup)
        }

        // MARK: Card 3 — Window & System
        addSectionHeader(tr("窗口与系统", "Window & System", "Janela e sistema"))
        addCard(rows: 8) { card, rh in
            // Row 0: Fullscreen topmost
            card.addSubview(makeLabel(tr("全屏游戏/应用置顶显示", "Always On Top Over Fullscreen", "Sempre visível sobre jogos/apps em tela cheia"), yPos: 4, width: 320))
            fullscreenOverlaySwitch = makeSwitch(s.fullScreenOverlay, action: #selector(onToggleFullscreenOverlay), yPos: 4)
            card.addSubview(fullscreenOverlaySwitch)
            addRowDivider(card, yPos: 4 + rh)

            // Row 1: Full click-through
            let r1 = 4 + rh
            card.addSubview(makeLabel(tr("完全鼠标穿透（按住 ⌥Option 临时拖动）", "Full Click-Through (Hold ⌥Option)", "Ignorar cliques (segure ⌥Option para arrastar)"), yPos: r1, width: 340))
            clickThroughSwitch = makeSwitch(s.fullClickThrough, action: #selector(onToggleClickThrough), yPos: r1)
            card.addSubview(clickThroughSwitch)
            addRowDivider(card, yPos: r1 + rh)

            // Row 2: Auto-hide in fullscreen
            let r2 = 4 + rh * 2
            card.addSubview(makeLabel(tr("全屏应用时自动隐藏", "Auto-Hide in Fullscreen Apps", "Ocultar em apps de tela cheia"), yPos: r2, width: 320))
            autoHideSwitch = makeSwitch(s.autoHideFullscreen, action: #selector(onToggleAutoHide), yPos: r2)
            card.addSubview(autoHideSwitch)
            addRowDivider(card, yPos: r2 + rh)

            // Row 3: OBS Capture Mode
            let r3 = 4 + rh * 3
            card.addSubview(makeLabel(tr("直播捕获模式 (OBS)", "OBS Capture Mode", "Captura para OBS"), yPos: r3, width: 150))
            obsPopup = NSPopUpButton(frame: NSRect(x: 170, y: r3 + 5, width: 272, height: 24), pullsDown: false)
            obsPopup.addItems(withTitles: [
                tr("关闭（正常桌面悬浮）", "Off (Normal Overlay)", "Desligado (sobreposição normal)"),
                tr("OBS 透明窗口捕获", "OBS Transparent Window", "OBS – janela transparente"),
                tr("OBS 绿幕模式 (#00FF00)", "OBS Green Screen (#00FF00)", "OBS – tela verde (#00FF00)")
            ])
            obsPopup.selectItem(at: max(0, min(2, s.obsCaptureMode)))
            obsPopup.target = self
            obsPopup.action = #selector(onOBSSelected)
            card.addSubview(obsPopup)
            addRowDivider(card, yPos: r3 + rh)

            // Row 4: OBS Hide Desktop
            let r4 = 4 + rh * 4
            card.addSubview(makeLabel(tr("OBS 后台捕获（隐藏桌面小猫）", "OBS Background Capture (Hide Desktop)", "Captura OBS em 2º plano (oculta na tela)"), yPos: r4, width: 320))
            obsHideSwitch = makeSwitch(s.obsHideDesktop, action: #selector(onToggleOBSHide), yPos: r4)
            obsHideSwitch.isEnabled = (s.obsCaptureMode > 0)
            card.addSubview(obsHideSwitch)
            addRowDivider(card, yPos: r4 + rh)

            // Row 5: Launch at Login
            let r5 = 4 + rh * 5
            card.addSubview(makeLabel(tr("开机自动启动", "Launch at Login", "Abrir ao iniciar sessão"), yPos: r5, width: 320))
            autoStartSwitch = makeSwitch(getAutoStart?() ?? false, action: #selector(onToggleAutoStart), yPos: r5)
            card.addSubview(autoStartSwitch)
            addRowDivider(card, yPos: r5 + rh)

            // Row 6: Hourly Announcement
            let r6 = 4 + rh * 6
            card.addSubview(makeLabel(tr("整点猫头报时（HH:00 弹框提示）", "Hourly Head-Toast Announcement (HH:00)", "Aviso de hora cheia (HH:00)"), yPos: r6, width: 340))
            hourlySwitch = makeSwitch(s.hourlyAnnouncement, action: #selector(onToggleHourlyAnnouncement), yPos: r6)
            card.addSubview(hourlySwitch)
            addRowDivider(card, yPos: r6 + rh)

            // Row 7: Language
            let r7 = 4 + rh * 7
            card.addSubview(makeLabel("语言 / Language / Idioma", yPos: r7, width: 180))
            langSeg = NSSegmentedControl(labels: ["简体中文", "English", "Português"], trackingMode: .selectOne, target: self, action: #selector(onLangChanged))
            langSeg.selectedSegment = Self.languageCodes.firstIndex(of: s.language) ?? 0
            langSeg.frame = NSRect(x: cardW - 14 - 246, y: r7 + 6, width: 246, height: 24)
            card.addSubview(langSeg)
        }

        doc.frame = NSRect(x: 0, y: 0, width: 492, height: y + 8)
        scroll.documentView = doc
        win.contentView = scroll
    }



    @objc private func onSkinSelected() {
        let idx = skinPopup.indexOfSelectedItem
        guard idx >= 0 && idx < skinIds.count else { return }
        let token = skinIds[idx]
        if token.hasPrefix("custom:") {
            CatSettings.shared.overrideSkinIris = false
            CatSettings.shared.customSkin = String(token.dropFirst(7))
        } else if token.hasPrefix("coat:"), let coat = CatCoatTheme(rawValue: String(token.dropFirst(5))) {
            CatSettings.shared.overrideSkinIris = false
            CatSettings.shared.customSkin = ""
            CatSettings.shared.coatTheme = coat
            switch coat {
            case .classicBlack: CatSettings.shared.eyeTheme = .neonChartreuse
            case .siamese: CatSettings.shared.eyeTheme = .siameseBlue
            case .shorthair: CatSettings.shared.eyeTheme = .softAmber
            case .orangeWhite: CatSettings.shared.eyeTheme = .referenceHazel
            }
            if let eIdx = EyeColorTheme.allCases.firstIndex(of: CatSettings.shared.eyeTheme) {
                eyePopup.selectItem(at: eIdx)
            }
        }
        onSettingsChanged?()
        rebuildUI()
    }

    @objc private func onTapRenameSkin() {
        let id = CatSettings.shared.customSkin
        guard !id.isEmpty else { return }
        onRenameSkin?(id)
    }

    @objc private func onTapDeleteSkin() {
        let id = CatSettings.shared.customSkin
        guard !id.isEmpty else { return }
        onDeleteSkin?(id)
    }

    @objc private func onKBSelected() {
        let idx = kbPopup.indexOfSelectedItem
        guard idx >= 0 && idx < kbIds.count else { return }
        let token = kbIds[idx]
        if token.hasPrefix("custom:") {
            CatSettings.shared.customKeyboard = String(token.dropFirst(7))
        } else if token.hasPrefix("theme:"), let kb = KeyboardTheme(rawValue: String(token.dropFirst(6))) {
            CatSettings.shared.customKeyboard = ""
            CatSettings.shared.keyboardTheme = kb
        }
        onSettingsChanged?()
    }

    @objc private func onEyeSelected() {
        let idx = eyePopup.indexOfSelectedItem
        guard idx >= 0 && idx < EyeColorTheme.allCases.count else { return }
        CatSettings.shared.overrideSkinIris = true
        CatSettings.shared.eyeTheme = EyeColorTheme.allCases[idx]
        onSettingsChanged?()
    }

    @objc private func onToggleWhiteOutline() {
        CatSettings.shared.whiteOutline = (whiteOutlineSwitch.state == .on)
        onSettingsChanged?()
    }

    @objc private func onToggleFlip() {
        CatSettings.shared.flipHorizontal = (flipSwitch.state == .on)
        UserDefaults.standard.synchronize()
        onSettingsChanged?()
    }

    @objc private func onRotationChanged() {
        var deg = CGFloat(rotSlider.doubleValue)
        for snap in [-180.0, -90.0, 0.0, 90.0, 180.0] as [CGFloat] {
            if abs(deg - snap) < 4.0 { deg = snap; break }
        }
        CatSettings.shared.rotationDeg = deg
        rotValLabel.stringValue = "\(Int(deg.rounded()))°"
        onSettingsChanged?()
    }

    @objc private func onResetRotation() {
        CatSettings.shared.rotationDeg = 0
        rotSlider.doubleValue = 0
        rotValLabel.stringValue = "0°"
        UserDefaults.standard.synchronize()
        onSettingsChanged?()
    }

    @objc private func onScaleSliderChanged() {
        let sc = CGFloat(scaleSlider.doubleValue)
        scaleValLabel.stringValue = "\(Int((sc * 100).rounded()))%"
        onScaleChanged?(sc)
    }

    @objc private func onTapResetPose() {
        CatSettings.shared.rotationDeg = 0
        CatSettings.shared.flipHorizontal = false
        UserDefaults.standard.synchronize()
        onResetPoseAndPosition?()
        rebuildUI()
    }

    @objc private func onTapDressingRoom() { onOpenDressingRoom?() }
    @objc private func onTapImportKB() { onImportKeyboardSkin?() }

    @objc private func onTapChooseSkinsDir() {
        guard let win = window else { return }
        let panel = NSOpenPanel()
        panel.title = tr("选择皮肤文件夹", "Choose Skins Folder", "Escolher pasta de skins")
        panel.prompt = tr("选择", "Choose", "Escolher")
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.allowsMultipleSelection = false
        panel.directoryURL = SkinLibrary.directory
        panel.beginSheetModal(for: win) { [weak self] response in
            guard response == .OK, let url = panel.url else { return }
            CatSettings.shared.customSkinsDirectory = url.path
            SkinLibrary.shared.reload()
            self?.onSkinsDirectoryChanged?()
            self?.onSettingsChanged?()
            self?.rebuildUI()
        }
    }

    @objc private func onTapOpenSkinsDir() {
        let dir = SkinLibrary.directory
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        NSWorkspace.shared.open(dir)
    }

    @objc private func onTapResetSkinsDir() {
        CatSettings.shared.customSkinsDirectory = ""
        SkinLibrary.shared.reload()
        onSkinsDirectoryChanged?()
        onSettingsChanged?()
        rebuildUI()
    }

    @objc private func onToggleDragAnim() {
        CatSettings.shared.enableDragAnimation = (dragAnimSwitch.state == .on)
        onSettingsChanged?()
    }

    @objc private func onToggleAnger() {
        CatSettings.shared.enableAnger = (angerSwitch.state == .on)
        onSettingsChanged?()
    }

    @objc private func onToggleBadges() {
        CatSettings.shared.showKeyBadges = (badgesSwitch.state == .on)
        onSettingsChanged?()
    }

    @objc private func onToggleMouseBadges() {
        CatSettings.shared.showMouseBadges = (mouseBadgesSwitch.state == .on)
        onSettingsChanged?()
    }

    @objc private func onTypingLimbSelected() {
        let idx = max(0, min(3, typingLimbPopup.indexOfSelectedItem))
        CatSettings.shared.footEasterEgg = (idx == 0 || idx == 1)
        CatSettings.shared.tailEasterEgg = (idx == 0 || idx == 2)
        onSettingsChanged?()
    }

    @objc private func onToggleLegends() {
        CatSettings.shared.showKeyLegends = (legendsSwitch.state == .on)
        onSettingsChanged?()
    }

    @objc private func onToggleMarquee() {
        CatSettings.shared.keyboardMarquee = (marqueeSwitch.state == .on)
        onSettingsChanged?()
    }

    @objc private func onToggleLevelKeyboard() {
        CatSettings.shared.levelKeyboard = (levelKeyboardSwitch.state == .on)
        onSettingsChanged?()
    }

    @objc private func onTabletSelected() {
        CatSettings.shared.tabletMode = max(0, min(2, tabletPopup.indexOfSelectedItem))
        onSettingsChanged?()
    }

    @objc private func onToggleFullscreenOverlay() {
        CatSettings.shared.fullScreenOverlay = (fullscreenOverlaySwitch.state == .on)
        onApplyWindowLevel?()
    }

    @objc private func onToggleClickThrough() {
        CatSettings.shared.fullClickThrough = (clickThroughSwitch.state == .on)
    }

    @objc private func onToggleAutoHide() {
        CatSettings.shared.autoHideFullscreen = (autoHideSwitch.state == .on)
        onAutoHideChanged?()
    }

    @objc private func onOBSSelected() {
        let mode = max(0, min(2, obsPopup.indexOfSelectedItem))
        CatSettings.shared.obsCaptureMode = mode
        if mode == 0 {
            CatSettings.shared.obsHideDesktop = false
            obsHideSwitch.state = .off
        }
        obsHideSwitch.isEnabled = (mode > 0)
        onApplyOBSMode?()
    }

    @objc private func onToggleOBSHide() {
        CatSettings.shared.obsHideDesktop = (obsHideSwitch.state == .on)
        onApplyOBSMode?()
    }

    @objc private func onToggleAutoStart() {
        setAutoStart?(autoStartSwitch.state == .on)
    }

    @objc private func onToggleHourlyAnnouncement() {
        CatSettings.shared.hourlyAnnouncement = (hourlySwitch.state == .on)
    }

    /// Segment order of the language picker.
    private static let languageCodes = ["zh", "en", "pt"]

    @objc private func onLangChanged() {
        CatSettings.shared.language = Self.languageCodes[max(0, min(langSeg.selectedSegment, Self.languageCodes.count - 1))]
        onLanguageChanged?()
        rebuildUI()
    }
}
