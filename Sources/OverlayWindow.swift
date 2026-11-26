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
        let usesDouniDesk = (skin.renderStyle == .douni || skin.renderStyle == .beagle)
        let keyboardVisible = usesDouniDesk ? (loaf < 0.99 && scruff < 0.99) : (scruff * 2.6 < 0.99)
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
        if usesDouniDesk && loaf > 0.85 {
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
    private static let fgColorSpace = CGColorSpace(name: CGColorSpace.sRGB) ?? CGColorSpaceCreateDeviceRGB()
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
    private var pendingDoubleClickToggle = false
    private var doubleClickDownScreenPt: CGPoint = .zero
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
    var forceClickThroughForModal = false

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
        if forceClickThroughForModal {
            if !win.ignoresMouseEvents {
                win.ignoresMouseEvents = true
            }
            setCursorIfNeeded(.arrow)
            return
        }
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
            let skipPetForHunt = ((CatSettings.shared.activeSkin.renderStyle == .douni || CatSettings.shared.activeSkin.renderStyle == .beagle) && physics.huntIntensity.value > 0.20)
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
        pendingDoubleClickToggle = false
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

            // Double-clicking toggles the clean resize & direction edit box; if the user keeps
            // holding and drags past 4pt on the second click, seamlessly start scruff-drag instead.
            if event.clickCount == 2 && (zone == .head || zone == .body || CatSettings.shared.isEditMode) {
                pendingDoubleClickToggle = true
                doubleClickDownScreenPt = screenPt
                dragStartScreenPt = screenPt
                dragStartWindowOrigin = win.frame.origin
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
        if pendingDoubleClickToggle {
            if hypot(currentScreenPt.x - doubleClickDownScreenPt.x, currentScreenPt.y - doubleClickDownScreenPt.y) >= 4.0 {
                pendingDoubleClickToggle = false
                isDragging = true
                physics.setDragging(true, dragDx: 0)
                setCursorIfNeeded(.closedHand)
            } else {
                return
            }
        }
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
        if pendingDoubleClickToggle {
            pendingDoubleClickToggle = false
            CatSettings.shared.isEditMode.toggle()
            needsDisplay = true
            return
        }
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

    func syncWindowLevel() {
        guard let window else { return }
        window.level = CatSettings.shared.fullScreenOverlay
            ? NSWindow.Level(rawValue: Int(CGWindowLevelForKey(.popUpMenuWindow)) + 1)
            : .floating
        window.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]
    }

    func present() {
        syncWindowLevel()
        refresh()
        NSApp.activate(ignoringOtherApps: true)
        showWindow(nil)
        window?.makeKeyAndOrderFront(nil)
        window?.orderFrontRegardless()
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

// MARK: - Unified KuroCat Settings & Batch Skin Manager (Minimal Pure-White Operate UI, Vector-Only)

private enum KuroUITheme {
    private static func adaptive(light: NSColor, dark: NSColor) -> NSColor {
        NSColor(name: nil) { appearance in
            let match = appearance.bestMatch(from: [.darkAqua, .aqua])
            return match == .darkAqua ? dark : light
        }
    }

    static let windowBg = adaptive(
        light: NSColor(srgbRed: 1.000, green: 1.000, blue: 1.000, alpha: 1), // #FFFFFF
        dark:  NSColor(srgbRed: 0.094, green: 0.094, blue: 0.102, alpha: 1)  // #18181A
    )
    static let cardBg = adaptive(
        light: NSColor(srgbRed: 1.000, green: 1.000, blue: 1.000, alpha: 1), // #FFFFFF
        dark:  NSColor(srgbRed: 0.125, green: 0.125, blue: 0.137, alpha: 1)  // #202023
    )
    static let surfaceMuted = adaptive(
        light: NSColor(srgbRed: 0.957, green: 0.957, blue: 0.965, alpha: 1), // #F4F4F6
        dark:  NSColor(srgbRed: 0.165, green: 0.165, blue: 0.180, alpha: 1)  // #2A2A2E
    )
    static let surfaceHover = adaptive(
        light: NSColor(srgbRed: 0.918, green: 0.918, blue: 0.933, alpha: 1), // #EAEAEF
        dark:  NSColor(srgbRed: 0.212, green: 0.212, blue: 0.231, alpha: 1)  // #36363B
    )
    static let border = adaptive(
        light: NSColor(srgbRed: 0.902, green: 0.902, blue: 0.922, alpha: 1), // #E6E6EB
        dark:  NSColor(srgbRed: 0.212, green: 0.212, blue: 0.235, alpha: 1)  // #36363C
    )
    static let divider = adaptive(
        light: NSColor(srgbRed: 0.941, green: 0.941, blue: 0.953, alpha: 1), // #F0F0F3
        dark:  NSColor(srgbRed: 0.176, green: 0.176, blue: 0.196, alpha: 1)  // #2D2D32
    )
    static let ink = adaptive(
        light: NSColor(srgbRed: 0.075, green: 0.075, blue: 0.086, alpha: 1), // #131316
        dark:  NSColor(srgbRed: 0.961, green: 0.961, blue: 0.969, alpha: 1)  // #F5F5F7
    )
    static let secondaryInk = adaptive(
        light: NSColor(srgbRed: 0.431, green: 0.431, blue: 0.451, alpha: 1), // #6E6E73
        dark:  NSColor(srgbRed: 0.596, green: 0.596, blue: 0.624, alpha: 1)  // #98989F
    )
    static let accent = adaptive(
        light: NSColor(srgbRed: 0.094, green: 0.094, blue: 0.106, alpha: 1), // #18181B
        dark:  NSColor(srgbRed: 0.941, green: 0.941, blue: 0.953, alpha: 1)  // #F0F0F3
    )
    static let accentForeground = adaptive(
        light: NSColor.white,
        dark:  NSColor(srgbRed: 0.075, green: 0.075, blue: 0.086, alpha: 1)
    )
    static let accentDown = adaptive(
        light: NSColor(srgbRed: 0.196, green: 0.196, blue: 0.216, alpha: 1), // #323237
        dark:  NSColor(srgbRed: 0.820, green: 0.820, blue: 0.840, alpha: 1)
    )
    static let accentSoft = adaptive(
        light: NSColor(srgbRed: 0.949, green: 0.949, blue: 0.961, alpha: 1), // #F2F2F5
        dark:  NSColor(srgbRed: 0.188, green: 0.188, blue: 0.212, alpha: 1)  // #303036
    )
    static let danger = adaptive(
        light: NSColor(srgbRed: 0.863, green: 0.149, blue: 0.149, alpha: 1), // #DC2626
        dark:  NSColor(srgbRed: 0.937, green: 0.267, blue: 0.267, alpha: 1)  // #EF4444
    )
    static let dangerSoft = adaptive(
        light: NSColor(srgbRed: 0.996, green: 0.949, blue: 0.949, alpha: 1), // #FEF2F2
        dark:  NSColor(srgbRed: 0.251, green: 0.118, blue: 0.122, alpha: 1)  // #401E1F
    )
    static let switchOff = adaptive(
        light: NSColor(srgbRed: 0.855, green: 0.855, blue: 0.878, alpha: 1), // #DADAE0
        dark:  NSColor(srgbRed: 0.267, green: 0.267, blue: 0.290, alpha: 1)  // #44444A
    )
    static let tabActiveBg = adaptive(
        light: NSColor.white,
        dark:  NSColor(srgbRed: 0.235, green: 0.235, blue: 0.259, alpha: 1)
    )
}

private final class SettingsCardView: NSView {
    var fillColor: NSColor = KuroUITheme.cardBg
    var strokeColor: NSColor = KuroUITheme.border
    var cornerRadius: CGFloat = 10

    override var isFlipped: Bool { true }
    override func draw(_ dirtyRect: NSRect) {
        let rect = bounds.insetBy(dx: 0.5, dy: 0.5)
        let path = NSBezierPath(roundedRect: rect, xRadius: cornerRadius, yRadius: cornerRadius)
        fillColor.setFill()
        path.fill()
        strokeColor.setStroke()
        path.lineWidth = 1.0
        path.stroke()
    }
}

private final class KuroBadgePillView: NSView {
    private let text: String
    private let bg: NSColor
    private let fg: NSColor
    override var isFlipped: Bool { true }

    init(frame: NSRect, text: String, bg: NSColor, fg: NSColor) {
        self.text = text
        self.bg = bg
        self.fg = fg
        super.init(frame: frame)
    }
    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }

    override func draw(_ dirtyRect: NSRect) {
        let path = NSBezierPath(roundedRect: bounds.insetBy(dx: 0.5, dy: 0.5), xRadius: 6, yRadius: 6)
        bg.setFill()
        path.fill()
        let attrs: [NSAttributedString.Key: Any] = [.font: NSFont.monospacedDigitSystemFont(ofSize: 10.5, weight: .medium), .foregroundColor: fg]
        let sz = (text as NSString).size(withAttributes: attrs)
        (text as NSString).draw(at: NSPoint(x: bounds.midX - sz.width * 0.5, y: bounds.midY - sz.height * 0.5), withAttributes: attrs)
    }
}

private final class KuroThemedButton: NSButton {
    enum Variant { case primary, secondary, danger, dangerSolid }

    var variant: Variant = .secondary { didSet { needsDisplay = true } }
    var fontSize: CGFloat = 12

    override func resetCursorRects() {
        if isEnabled { addCursorRect(bounds, cursor: .pointingHand) }
    }

    override func draw(_ dirtyRect: NSRect) {
        let r = bounds.insetBy(dx: 0.5, dy: 0.5)
        let rad: CGFloat = 7
        let path = NSBezierPath(roundedRect: r, xRadius: rad, yRadius: rad)

        let bg: NSColor
        let fg: NSColor
        var stroke: NSColor? = nil
        if !isEnabled {
            bg = KuroUITheme.surfaceMuted.withAlphaComponent(0.6)
            fg = KuroUITheme.secondaryInk.withAlphaComponent(0.45)
            stroke = KuroUITheme.border.withAlphaComponent(0.6)
        } else {
            switch variant {
            case .primary:
                bg = isHighlighted ? KuroUITheme.accentDown : KuroUITheme.accent
                fg = KuroUITheme.accentForeground
            case .secondary:
                bg = isHighlighted ? KuroUITheme.surfaceHover : KuroUITheme.cardBg
                fg = KuroUITheme.ink
                stroke = KuroUITheme.border
            case .danger:
                bg = isHighlighted ? KuroUITheme.danger.withAlphaComponent(0.14) : KuroUITheme.cardBg
                fg = KuroUITheme.danger
                stroke = KuroUITheme.danger.withAlphaComponent(0.32)
            case .dangerSolid:
                bg = isHighlighted ? KuroUITheme.danger.blended(withFraction: 0.16, of: .black) ?? KuroUITheme.danger : KuroUITheme.danger
                fg = .white
            }
        }
        bg.setFill()
        path.fill()
        if let stroke {
            stroke.setStroke()
            path.lineWidth = 1.0
            path.stroke()
        }

        let attrs: [NSAttributedString.Key: Any] = [
            .font: NSFont.systemFont(ofSize: fontSize, weight: (variant == .primary || variant == .dangerSolid) ? .medium : .regular),
            .foregroundColor: fg
        ]
        let sz = (title as NSString).size(withAttributes: attrs)
        (title as NSString).draw(at: NSPoint(x: bounds.midX - sz.width * 0.5, y: bounds.midY - sz.height * 0.5 - 0.5), withAttributes: attrs)
    }
}

/// Minimal monochrome vector toggle switch (zero bitmap overhead).
private final class KuroSwitchControl: NSControl {
    var isOn: Bool = false { didSet { needsDisplay = true } }
    override var isEnabled: Bool { didSet { needsDisplay = true } }
    override var isFlipped: Bool { true }

    init(isOn: Bool, target: AnyObject?, action: Selector?) {
        self.isOn = isOn
        super.init(frame: NSRect(x: 0, y: 0, width: 36, height: 20))
        self.target = target
        self.action = action
    }
    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }

    override func resetCursorRects() {
        if isEnabled { addCursorRect(bounds, cursor: .pointingHand) }
    }

    override func mouseDown(with event: NSEvent) {
        guard isEnabled else { return }
        isOn.toggle()
        if let action { NSApp.sendAction(action, to: target, from: self) }
    }

    override func draw(_ dirtyRect: NSRect) {
        let track = bounds.insetBy(dx: 0.5, dy: 0.5)
        let path = NSBezierPath(roundedRect: track, xRadius: track.height * 0.5, yRadius: track.height * 0.5)
        let trackColor = isOn ? KuroUITheme.accent : KuroUITheme.switchOff
        (isEnabled ? trackColor : trackColor.withAlphaComponent(0.45)).setFill()
        path.fill()

        let d: CGFloat = bounds.height - 4
        let knobX = isOn ? (bounds.width - d - 2) : 2
        let knob = NSBezierPath(ovalIn: NSRect(x: knobX, y: 2, width: d, height: d))
        let knobColor = isOn ? KuroUITheme.accentForeground : NSColor.white
        knobColor.withAlphaComponent(isEnabled ? 0.98 : 0.72).setFill()
        knob.fill()
    }
}

private final class KuroTabBarView: NSView {
    var titles: [String] = [] { didSet { needsDisplay = true } }
    var selectedIndex: Int = 0 { didSet { needsDisplay = true } }
    var onSelect: ((Int) -> Void)?
    override var isFlipped: Bool { true }

    override func resetCursorRects() {
        addCursorRect(bounds, cursor: .pointingHand)
    }

    override func mouseDown(with event: NSEvent) {
        guard !titles.isEmpty else { return }
        let pt = convert(event.locationInWindow, from: nil)
        let segW = bounds.width / CGFloat(titles.count)
        let idx = max(0, min(titles.count - 1, Int(pt.x / segW)))
        if idx != selectedIndex {
            selectedIndex = idx
            onSelect?(idx)
        }
    }

    override func draw(_ dirtyRect: NSRect) {
        let bg = NSBezierPath(roundedRect: bounds.insetBy(dx: 0.5, dy: 0.5), xRadius: 9, yRadius: 9)
        KuroUITheme.surfaceMuted.setFill()
        bg.fill()

        guard !titles.isEmpty else { return }
        let pad: CGFloat = 3
        let segW = (bounds.width - pad * 2) / CGFloat(titles.count)
        for (i, title) in titles.enumerated() {
            let segRect = NSRect(x: pad + CGFloat(i) * segW, y: pad, width: segW, height: bounds.height - pad * 2)
            let active = (i == selectedIndex)
            if active {
                NSGraphicsContext.saveGraphicsState()
                let shadow = NSShadow()
                shadow.shadowColor = NSColor.black.withAlphaComponent(0.08)
                shadow.shadowOffset = NSSize(width: 0, height: -1)
                shadow.shadowBlurRadius = 2.5
                shadow.set()
                let pill = NSBezierPath(roundedRect: segRect.insetBy(dx: 0.5, dy: 0.5), xRadius: 6.5, yRadius: 6.5)
                KuroUITheme.tabActiveBg.setFill()
                pill.fill()
                NSGraphicsContext.restoreGraphicsState()

                let pillStroke = NSBezierPath(roundedRect: segRect.insetBy(dx: 0.5, dy: 0.5), xRadius: 6.5, yRadius: 6.5)
                KuroUITheme.border.withAlphaComponent(0.8).setStroke()
                pillStroke.lineWidth = 0.75
                pillStroke.stroke()
            }
            let attrs: [NSAttributedString.Key: Any] = [
                .font: NSFont.systemFont(ofSize: 12.5, weight: active ? .semibold : .regular),
                .foregroundColor: active ? KuroUITheme.ink : KuroUITheme.secondaryInk
            ]
            let sz = (title as NSString).size(withAttributes: attrs)
            (title as NSString).draw(at: NSPoint(x: segRect.midX - sz.width * 0.5, y: segRect.midY - sz.height * 0.5 - 0.5), withAttributes: attrs)
        }
    }
}

/// Consolidated KuroCat Preferences window (warm light/dark adaptive cards, 3 tabs, releases contentView on close).
final class SettingsWindowController: NSWindowController, NSWindowDelegate {
    var onSettingsChanged: (() -> Void)?
    var onScaleChanged: ((CGFloat) -> Void)?
    var onResetPoseAndPosition: (() -> Void)?
    var onOpenDressingRoom: (() -> Void)?
    var onOpenSkinManager: (() -> Void)?
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

    private var activeTab: Int = 0
    private var skinPopup: NSPopUpButton?
    private var skinsDirLabel: NSTextField?
    private var resetSkinsDirBtn: KuroThemedButton?
    private var kbPopup: NSPopUpButton?
    private var eyePopup: NSPopUpButton?
    private var tabletPopup: NSPopUpButton?
    private var typingLimbPopup: NSPopUpButton?
    private var obsPopup: NSPopUpButton?
    private var langSeg: NSSegmentedControl?

    private var whiteOutlineSwitch: KuroSwitchControl?
    private var flipSwitch: KuroSwitchControl?
    private var angerSwitch: KuroSwitchControl?
    private var dragAnimSwitch: KuroSwitchControl?
    private var badgesSwitch: KuroSwitchControl?
    private var mouseBadgesSwitch: KuroSwitchControl?
    private var legendsSwitch: KuroSwitchControl?
    private var marqueeSwitch: KuroSwitchControl?
    private var levelKeyboardSwitch: KuroSwitchControl?
    private var fullscreenOverlaySwitch: KuroSwitchControl?
    private var clickThroughSwitch: KuroSwitchControl?
    private var autoHideSwitch: KuroSwitchControl?
    private var autoHeadphonesSwitch: KuroSwitchControl?
    private var obsHideSwitch: KuroSwitchControl?
    private var autoStartSwitch: KuroSwitchControl?
    private var hourlySwitch: KuroSwitchControl?

    private var rotSlider: NSSlider?
    private var rotValLabel: NSTextField?
    private var scaleSlider: NSSlider?
    private var scaleValLabel: NSTextField?

    private var skinIds: [String] = []
    private var kbIds: [String] = []

    init() {
        let win = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 520, height: 548),
            styleMask: [.titled, .closable],
            backing: .buffered,
            defer: false
        )
        win.isReleasedWhenClosed = false
        win.level = .floating
        win.titlebarAppearsTransparent = true
        win.isMovableByWindowBackground = true
        win.backgroundColor = KuroUITheme.windowBg
        super.init(window: win)
        win.delegate = self
        win.center()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func tr(_ zh: String, _ en: String, _ pt: String) -> String {
        loc(zh, en, pt)
    }

    func windowWillClose(_ notification: Notification) {
        // Drop the view hierarchy & backing layers when closed so idle memory stays near zero.
        clearControlRefs()
        window?.contentView = nil
    }

    private func clearControlRefs() {
        skinPopup = nil; skinsDirLabel = nil; resetSkinsDirBtn = nil
        kbPopup = nil; eyePopup = nil; tabletPopup = nil; typingLimbPopup = nil
        obsPopup = nil; langSeg = nil
        whiteOutlineSwitch = nil; flipSwitch = nil; angerSwitch = nil; dragAnimSwitch = nil
        badgesSwitch = nil; mouseBadgesSwitch = nil; legendsSwitch = nil; marqueeSwitch = nil
        levelKeyboardSwitch = nil; fullscreenOverlaySwitch = nil; clickThroughSwitch = nil
        autoHideSwitch = nil; autoHeadphonesSwitch = nil; obsHideSwitch = nil
        autoStartSwitch = nil; hourlySwitch = nil
        rotSlider = nil; rotValLabel = nil; scaleSlider = nil; scaleValLabel = nil
    }

    func syncWindowLevel() {
        guard let window else { return }
        window.level = CatSettings.shared.fullScreenOverlay
            ? NSWindow.Level(rawValue: Int(CGWindowLevelForKey(.popUpMenuWindow)) + 1)
            : .floating
        window.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]
    }

    func present() {
        syncWindowLevel()
        rebuildUI()
        NSApp.activate(ignoringOtherApps: true)
        showWindow(nil)
        window?.makeKeyAndOrderFront(nil)
        window?.orderFrontRegardless()
    }

    func refresh() {
        guard window?.isVisible == true else { return }
        rebuildUI()
    }

    private func makeBtn(_ title: String, variant: KuroThemedButton.Variant, action: Selector, frame: NSRect, fontSize: CGFloat = 11.5) -> KuroThemedButton {
        let btn = KuroThemedButton(title: title, target: self, action: action)
        btn.isBordered = false
        btn.variant = variant
        btn.fontSize = fontSize
        btn.frame = frame
        return btn
    }

    private func rebuildUI() {
        guard let win = window else { return }
        clearControlRefs()
        let s = CatSettings.shared
        win.title = tr("KuroCat 设置", "KuroCat Settings", "Configurações do KuroCat")
        win.backgroundColor = KuroUITheme.windowBg

        let winW: CGFloat = 520
        let winH: CGFloat = 548
        let padX: CGFloat = 20
        let cardW: CGFloat = winW - padX * 2

        let root = FlippedView(frame: NSRect(x: 0, y: 0, width: winW, height: winH))
        let ver = Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String ?? "1.5.2"

        // MARK: Minimal Typographic Header (no logo, no bulky banner card)
        let titleLbl = NSTextField(labelWithString: tr("KuroCat 设置", "KuroCat Settings", "Configurações do KuroCat"))
        titleLbl.font = .systemFont(ofSize: 17, weight: .semibold)
        titleLbl.textColor = KuroUITheme.ink
        titleLbl.frame = NSRect(x: padX, y: 10, width: 260, height: 22)
        root.addSubview(titleLbl)

        let activeName = s.activeSkin.name
        let customCount = SkinLibrary.shared.skins.count
        let subText = customCount > 0
            ? tr("当前皮肤：\(activeName) · 已导入 \(customCount) 款", "Active: \(activeName) · \(customCount) imported", "Em uso: \(activeName) · \(customCount) importada(s)")
            : tr("当前皮肤：\(activeName)", "Active: \(activeName)", "Em uso: \(activeName)")
        let subLbl = NSTextField(labelWithString: subText)
        subLbl.font = .systemFont(ofSize: 12)
        subLbl.textColor = KuroUITheme.secondaryInk
        subLbl.lineBreakMode = .byTruncatingTail
        subLbl.frame = NSRect(x: padX, y: 34, width: cardW - 70, height: 16)
        root.addSubview(subLbl)

        let verPill = KuroBadgePillView(
            frame: NSRect(x: winW - padX - 52, y: 12, width: 52, height: 20),
            text: "v\(ver)",
            bg: KuroUITheme.surfaceMuted,
            fg: KuroUITheme.secondaryInk
        )
        root.addSubview(verPill)

        // MARK: 3-Tab Minimal Segmented Switcher (no emoji)
        let tabBar = KuroTabBarView(frame: NSRect(x: padX, y: 62, width: cardW, height: 32))
        tabBar.titles = [
            tr("外观", "Appearance", "Aparência"),
            tr("互动", "Interaction", "Interação"),
            tr("系统", "System", "Sistema")
        ]
        tabBar.selectedIndex = activeTab
        tabBar.onSelect = { [weak self] idx in
            self?.activeTab = idx
            self?.rebuildUI()
        }
        root.addSubview(tabBar)

        // MARK: Scrollable Body for Active Tab
        let bodyY: CGFloat = 106
        let bodyH: CGFloat = winH - bodyY - 56
        let scroll = NSScrollView(frame: NSRect(x: 0, y: bodyY, width: winW, height: bodyH))
        scroll.hasVerticalScroller = true
        scroll.drawsBackground = false
        scroll.borderType = .noBorder

        let doc = FlippedView(frame: NSRect(x: 0, y: 0, width: winW, height: bodyH))
        var y: CGFloat = 4
        let rowH: CGFloat = 41

        func addCard(rows: Int, builder: (NSView, CGFloat) -> Void) {
            let h = CGFloat(rows) * rowH + 6
            let card = SettingsCardView(frame: NSRect(x: padX, y: y, width: cardW, height: h))
            builder(card, rowH)
            doc.addSubview(card)
            y += h + 10
        }

        func addRowDivider(_ card: NSView, yPos: CGFloat) {
            let div = NSView(frame: NSRect(x: 16, y: yPos, width: cardW - 32, height: 1))
            div.wantsLayer = true
            div.layer?.backgroundColor = KuroUITheme.divider.cgColor
            card.addSubview(div)
        }

        func makeLabel(_ text: String, yPos: CGFloat, width: CGFloat = 260) -> NSTextField {
            let lbl = NSTextField(labelWithString: text)
            lbl.font = .systemFont(ofSize: 13, weight: .regular)
            lbl.textColor = KuroUITheme.ink
            lbl.frame = NSRect(x: 16, y: yPos + 11, width: width, height: 19)
            return lbl
        }

        func makeSwitch(_ on: Bool, action: Selector, yPos: CGFloat) -> KuroSwitchControl {
            let sw = KuroSwitchControl(isOn: on, target: self, action: action)
            sw.frame = NSRect(x: cardW - 16 - 36, y: yPos + (rowH - 20) * 0.5, width: 36, height: 20)
            return sw
        }

        if activeTab == 0 {
            addCard(rows: 8) { card, rh in
                // Row 0: Cat Skin + inline Rename / Delete (only when custom skin active)
                let hasCustomActive = !s.customSkin.isEmpty && SkinLibrary.shared.skin(id: s.customSkin) != nil
                card.addSubview(makeLabel(tr("猫咪皮肤", "Cat Skin", "Skin do gato"), yPos: 3, width: 80))
                let popup = NSPopUpButton(frame: NSRect(x: 104, y: 9, width: hasCustomActive ? 248 : 360, height: 24), pullsDown: false)
                skinIds = []
                for coat in CatCoatTheme.allCases {
                    popup.addItem(withTitle: coat.displayName)
                    skinIds.append("coat:\(coat.rawValue)")
                }
                for sk in SkinLibrary.shared.skins {
                    popup.addItem(withTitle: SkinLibrary.shared.displayName(for: sk))
                    skinIds.append("custom:\(sk.id)")
                }
                if !s.customSkin.isEmpty, let idx = skinIds.firstIndex(of: "custom:\(s.customSkin)") {
                    popup.selectItem(at: idx)
                } else if let idx = skinIds.firstIndex(of: "coat:\(s.coatTheme.rawValue)") {
                    popup.selectItem(at: idx)
                }
                popup.target = self
                popup.action = #selector(onSkinSelected)
                card.addSubview(popup)
                skinPopup = popup

                if hasCustomActive {
                    card.addSubview(makeBtn(tr("改名", "Rename", "Renomear"), variant: .secondary, action: #selector(onTapRenameSkin), frame: NSRect(x: 358, y: 9, width: 50, height: 24)))
                    card.addSubview(makeBtn(tr("删除", "Delete", "Excluir"), variant: .danger, action: #selector(onTapDeleteSkin), frame: NSRect(x: 414, y: 9, width: 50, height: 24)))
                }
                addRowDivider(card, yPos: 3 + rh)

                // Row 1: Skins Folder path + Choose / Open / Reset
                let r1 = 3 + rh
                card.addSubview(makeLabel(tr("皮肤目录", "Skins Dir", "Pasta"), yPos: r1, width: 80))
                let dirDisplay = (SkinLibrary.directory.path as NSString).abbreviatingWithTildeInPath
                let dirLbl = NSTextField(labelWithString: dirDisplay)
                dirLbl.font = .systemFont(ofSize: 11.5)
                dirLbl.textColor = KuroUITheme.secondaryInk
                dirLbl.lineBreakMode = .byTruncatingMiddle
                dirLbl.toolTip = SkinLibrary.directory.path
                dirLbl.frame = NSRect(x: 104, y: r1 + 12, width: 162, height: 17)
                card.addSubview(dirLbl)
                skinsDirLabel = dirLbl

                card.addSubview(makeBtn(tr("更改", "Choose", "Alterar"), variant: .secondary, action: #selector(onTapChooseSkinsDir), frame: NSRect(x: 272, y: r1 + 8, width: 56, height: 24)))
                card.addSubview(makeBtn(tr("打开", "Open", "Abrir"), variant: .secondary, action: #selector(onTapOpenSkinsDir), frame: NSRect(x: 334, y: r1 + 8, width: 56, height: 24)))
                let rstDirBtn = makeBtn(tr("默认", "Reset", "Padrão"), variant: .secondary, action: #selector(onTapResetSkinsDir), frame: NSRect(x: 396, y: r1 + 8, width: 68, height: 24))
                rstDirBtn.isEnabled = SkinLibrary.isUsingCustomDirectory
                card.addSubview(rstDirBtn)
                resetSkinsDirBtn = rstDirBtn
                addRowDivider(card, yPos: r1 + rh)

                // Row 2: Keyboard Theme + Import
                let r2 = 3 + rh * 2
                card.addSubview(makeLabel(tr("键盘配色", "Keyboard", "Teclado"), yPos: r2, width: 80))
                let kbP = NSPopUpButton(frame: NSRect(x: 104, y: r2 + 8, width: 274, height: 24), pullsDown: false)
                kbIds = []
                KeyboardSkinLibrary.shared.reload()
                for kb in KeyboardTheme.allCases {
                    kbP.addItem(withTitle: kb.displayName)
                    kbIds.append("theme:\(kb.rawValue)")
                }
                for kb in KeyboardSkinLibrary.shared.skins {
                    kbP.addItem(withTitle: kb.name)
                    kbIds.append("custom:\(kb.id)")
                }
                if !s.customKeyboard.isEmpty, let idx = kbIds.firstIndex(of: "custom:\(s.customKeyboard)") {
                    kbP.selectItem(at: idx)
                } else if let idx = kbIds.firstIndex(of: "theme:\(s.keyboardTheme.rawValue)") {
                    kbP.selectItem(at: idx)
                }
                kbP.target = self
                kbP.action = #selector(onKBSelected)
                card.addSubview(kbP)
                kbPopup = kbP
                card.addSubview(makeBtn(tr("导入…", "Import…", "Importar…"), variant: .secondary, action: #selector(onTapImportKB), frame: NSRect(x: 386, y: r2 + 8, width: 78, height: 24)))
                addRowDivider(card, yPos: r2 + rh)

                // Row 3: Eye Color
                let r3 = 3 + rh * 3
                card.addSubview(makeLabel(tr("瞳色", "Eye Color", "Cor dos olhos"), yPos: r3, width: 80))
                let eyeP = NSPopUpButton(frame: NSRect(x: 104, y: r3 + 8, width: 360, height: 24), pullsDown: false)
                for eye in EyeColorTheme.allCases {
                    eyeP.addItem(withTitle: eye.displayName)
                }
                if let idx = EyeColorTheme.allCases.firstIndex(of: s.eyeTheme) { eyeP.selectItem(at: idx) }
                eyeP.target = self
                eyeP.action = #selector(onEyeSelected)
                card.addSubview(eyeP)
                eyePopup = eyeP
                addRowDivider(card, yPos: r3 + rh)

                // Row 4: White Outline toggle
                let r4 = 3 + rh * 4
                card.addSubview(makeLabel(tr("白色外描边（贴纸风）", "White Sticker Outline", "Contorno branco (adesivo)"), yPos: r4, width: 340))
                let outlineSw = makeSwitch(s.whiteOutline, action: #selector(onToggleWhiteOutline), yPos: r4)
                card.addSubview(outlineSw)
                whiteOutlineSwitch = outlineSw
                addRowDivider(card, yPos: r4 + rh)

                // Row 5: Horizontal Flip toggle
                let r5 = 3 + rh * 5
                card.addSubview(makeLabel(tr("水平镜像翻转", "Flip Horizontal", "Espelhar horizontalmente"), yPos: r5, width: 340))
                let flipSw = makeSwitch(s.flipHorizontal, action: #selector(onToggleFlip), yPos: r5)
                card.addSubview(flipSw)
                flipSwitch = flipSw
                addRowDivider(card, yPos: r5 + rh)

                // Row 6: Rotation Angle slider (monochrome track fill matches theme)
                let r6 = 3 + rh * 6
                card.addSubview(makeLabel(tr("旋转角度", "Rotation", "Rotação"), yPos: r6, width: 80))
                let rSl = NSSlider(value: Double(s.rotationDeg), minValue: -180, maxValue: 180, target: self, action: #selector(onRotationChanged))
                rSl.trackFillColor = KuroUITheme.accent
                rSl.frame = NSRect(x: 104, y: r6 + 10, width: 226, height: 20)
                card.addSubview(rSl)
                rotSlider = rSl
                let rVal = NSTextField(labelWithString: "\(Int(s.rotationDeg.rounded()))°")
                rVal.font = .monospacedDigitSystemFont(ofSize: 12, weight: .medium)
                rVal.textColor = KuroUITheme.secondaryInk
                rVal.alignment = .right
                rVal.frame = NSRect(x: 334, y: r6 + 11, width: 44, height: 18)
                card.addSubview(rVal)
                rotValLabel = rVal
                card.addSubview(makeBtn(tr("回正 0°", "Reset 0°", "Voltar a 0°"), variant: .secondary, action: #selector(onResetRotation), frame: NSRect(x: 386, y: r6 + 8, width: 78, height: 24)))
                addRowDivider(card, yPos: r6 + rh)

                // Row 7: Scale slider
                let r7 = 3 + rh * 7
                card.addSubview(makeLabel(tr("桌宠大小", "Scale", "Tamanho"), yPos: r7, width: 80))
                let sSl = NSSlider(value: Double(s.scale), minValue: 0.45, maxValue: 2.0, target: self, action: #selector(onScaleSliderChanged))
                sSl.trackFillColor = KuroUITheme.accent
                sSl.frame = NSRect(x: 104, y: r7 + 10, width: 226, height: 20)
                card.addSubview(sSl)
                scaleSlider = sSl
                let sVal = NSTextField(labelWithString: "\(Int((s.scale * 100).rounded()))%")
                sVal.font = .monospacedDigitSystemFont(ofSize: 12, weight: .medium)
                sVal.textColor = KuroUITheme.secondaryInk
                sVal.alignment = .right
                sVal.frame = NSRect(x: 334, y: r7 + 11, width: 44, height: 18)
                card.addSubview(sVal)
                scaleValLabel = sVal
                card.addSubview(makeBtn(tr("重置位置", "Reset", "Redefinir"), variant: .secondary, action: #selector(onTapResetPose), frame: NSRect(x: 386, y: r7 + 8, width: 78, height: 24)))
            }
        } else if activeTab == 1 {
            addCard(rows: 9) { card, rh in
                card.addSubview(makeLabel(tr("拎起拖拽动画", "Scruff-Drag Lift Animation", "Animação de erguer ao arrastar"), yPos: 3, width: 360))
                let dragSw = makeSwitch(s.enableDragAnimation, action: #selector(onToggleDragAnim), yPos: 3)
                card.addSubview(dragSw); dragAnimSwitch = dragSw
                addRowDivider(card, yPos: 3 + rh)

                let r1 = 3 + rh
                card.addSubview(makeLabel(tr("打字过快时生气表情", "Angry Expression on Fast Typing", "Cara brava ao digitar rápido demais"), yPos: r1, width: 360))
                let angerSw = makeSwitch(s.enableAnger, action: #selector(onToggleAnger), yPos: r1)
                card.addSubview(angerSw); angerSwitch = angerSw
                addRowDivider(card, yPos: r1 + rh)

                let r2 = 3 + rh * 2
                card.addSubview(makeLabel(tr("显示按键气泡", "Show Keyboard Key Badges", "Mostrar balões das teclas"), yPos: r2, width: 360))
                let badgesSw = makeSwitch(s.showKeyBadges, action: #selector(onToggleBadges), yPos: r2)
                card.addSubview(badgesSw); badgesSwitch = badgesSw
                addRowDivider(card, yPos: r2 + rh)

                let r3 = 3 + rh * 3
                card.addSubview(makeLabel(tr("显示鼠标点击提示", "Show Mouse Click Badges", "Mostrar cliques do mouse"), yPos: r3, width: 360))
                let mBadgesSw = makeSwitch(s.showMouseBadges, action: #selector(onToggleMouseBadges), yPos: r3)
                card.addSubview(mBadgesSw); mouseBadgesSwitch = mBadgesSw
                addRowDivider(card, yPos: r3 + rh)

                let r4 = 3 + rh * 4
                card.addSubview(makeLabel(tr("多键打字方式", "Multi-Key Typing", "Várias teclas"), yPos: r4, width: 120))
                let limbP = NSPopUpButton(frame: NSRect(x: 144, y: r4 + 8, width: 320, height: 24), pullsDown: false)
                limbP.addItems(withTitles: [
                    tr("后脚 + 尾巴（3键后脚 / 4键尾巴）", "Hind Foot + Tail (3-Key / 4-Key)", "Pata traseira + cauda (3 / 4 teclas)"),
                    tr("仅后脚打字", "3rd Hind Foot Only", "Só a pata traseira"),
                    tr("仅尾巴打字", "Tail Typing Only", "Só a cauda"),
                    tr("仅两只前爪打字", "Two Front Paws Only", "Só as patas da frente")
                ])
                let limbIdx: Int = s.footEasterEgg ? (s.tailEasterEgg ? 0 : 1) : (s.tailEasterEgg ? 2 : 3)
                limbP.selectItem(at: limbIdx)
                limbP.target = self
                limbP.action = #selector(onTypingLimbSelected)
                card.addSubview(limbP); typingLimbPopup = limbP
                addRowDivider(card, yPos: r4 + rh)

                let r5 = 3 + rh * 5
                card.addSubview(makeLabel(tr("显示键盘字符", "Show Keycap Legends", "Mostrar letras nas teclas"), yPos: r5, width: 360))
                let legSw = makeSwitch(s.showKeyLegends, action: #selector(onToggleLegends), yPos: r5)
                card.addSubview(legSw); legendsSwitch = legSw
                addRowDivider(card, yPos: r5 + rh)

                let r6 = 3 + rh * 6
                card.addSubview(makeLabel(tr("键盘跑马灯流光", "Keyboard RGB Marquee", "Luz RGB animada no teclado"), yPos: r6, width: 360))
                let marqSw = makeSwitch(s.keyboardMarquee, action: #selector(onToggleMarquee), yPos: r6)
                card.addSubview(marqSw); marqueeSwitch = marqSw
                addRowDivider(card, yPos: r6 + rh)

                let r7 = 3 + rh * 7
                card.addSubview(makeLabel(tr("键盘水平摆放", "Level Keyboard", "Teclado nivelado"), yPos: r7, width: 360))
                let lvlSw = makeSwitch(s.levelKeyboard, action: #selector(onToggleLevelKeyboard), yPos: r7)
                card.addSubview(lvlSw); levelKeyboardSwitch = lvlSw
                addRowDivider(card, yPos: r7 + rh)

                let r8 = 3 + rh * 8
                card.addSubview(makeLabel(tr("数位板与触控笔", "Tablet & Stylus", "Mesa e caneta"), yPos: r8, width: 120))
                let tabP = NSPopUpButton(frame: NSRect(x: 144, y: r8 + 8, width: 320, height: 24), pullsDown: false)
                tabP.addItems(withTitles: [
                    tr("自动检测", "Auto-Detect", "Detectar automaticamente"),
                    tr("始终显示数位板", "Always Show Tablet", "Sempre mostrar mesa digitalizadora"),
                    tr("始终显示鼠标", "Always Show Mouse", "Sempre mostrar mouse")
                ])
                tabP.selectItem(at: max(0, min(2, s.tabletMode)))
                tabP.target = self
                tabP.action = #selector(onTabletSelected)
                card.addSubview(tabP); tabletPopup = tabP
            }
        } else {
            addCard(rows: 9) { card, rh in
                card.addSubview(makeLabel(tr("全屏应用上方置顶显示", "Always On Top Over Fullscreen", "Sempre visível sobre tela cheia"), yPos: 3, width: 360))
                let fsSw = makeSwitch(s.fullScreenOverlay, action: #selector(onToggleFullscreenOverlay), yPos: 3)
                card.addSubview(fsSw); fullscreenOverlaySwitch = fsSw
                addRowDivider(card, yPos: 3 + rh)

                let r1 = 3 + rh
                card.addSubview(makeLabel(tr("完全鼠标穿透（按住 ⌥Option 拖动）", "Full Click-Through (Hold ⌥Option)", "Ignorar cliques (segure ⌥Option)"), yPos: r1, width: 360))
                let ctSw = makeSwitch(s.fullClickThrough, action: #selector(onToggleClickThrough), yPos: r1)
                card.addSubview(ctSw); clickThroughSwitch = ctSw
                addRowDivider(card, yPos: r1 + rh)

                let r2 = 3 + rh * 2
                card.addSubview(makeLabel(tr("全屏应用时自动隐藏", "Auto-Hide in Fullscreen Apps", "Ocultar em apps de tela cheia"), yPos: r2, width: 360))
                let ahSw = makeSwitch(s.autoHideFullscreen, action: #selector(onToggleAutoHide), yPos: r2)
                card.addSubview(ahSw); autoHideSwitch = ahSw
                addRowDivider(card, yPos: r2 + rh)

                let r3 = 3 + rh * 3
                card.addSubview(makeLabel(tr("OBS 直播捕获", "OBS Capture", "Captura OBS"), yPos: r3, width: 120))
                let obsP = NSPopUpButton(frame: NSRect(x: 144, y: r3 + 8, width: 320, height: 24), pullsDown: false)
                obsP.addItems(withTitles: [
                    tr("关闭（正常悬浮）", "Off (Normal Overlay)", "Desligado (sobreposição normal)"),
                    tr("OBS 透明窗口捕获", "OBS Transparent Window", "OBS – janela transparente"),
                    tr("OBS 绿幕模式 (#00FF00)", "OBS Green Screen (#00FF00)", "OBS – tela verde (#00FF00)")
                ])
                obsP.selectItem(at: max(0, min(2, s.obsCaptureMode)))
                obsP.target = self
                obsP.action = #selector(onOBSSelected)
                card.addSubview(obsP); obsPopup = obsP
                addRowDivider(card, yPos: r3 + rh)

                let r4 = 3 + rh * 4
                card.addSubview(makeLabel(tr("OBS 后台捕获（隐藏桌面小猫）", "OBS Background Capture", "Captura OBS em 2º plano"), yPos: r4, width: 360))
                let ohSw = makeSwitch(s.obsHideDesktop, action: #selector(onToggleOBSHide), yPos: r4)
                ohSw.isEnabled = (s.obsCaptureMode > 0)
                card.addSubview(ohSw); obsHideSwitch = ohSw
                addRowDivider(card, yPos: r4 + rh)

                let r5 = 3 + rh * 5
                card.addSubview(makeLabel(tr("开机自动启动", "Launch at Login", "Abrir ao iniciar sessão"), yPos: r5, width: 360))
                let asSw = makeSwitch(getAutoStart?() ?? false, action: #selector(onToggleAutoStart), yPos: r5)
                card.addSubview(asSw); autoStartSwitch = asSw
                addRowDivider(card, yPos: r5 + rh)

                let r6 = 3 + rh * 6
                card.addSubview(makeLabel(tr("整点猫头报时", "Hourly Announcement", "Aviso de hora cheia"), yPos: r6, width: 360))
                let hrSw = makeSwitch(s.hourlyAnnouncement, action: #selector(onToggleHourlyAnnouncement), yPos: r6)
                card.addSubview(hrSw); hourlySwitch = hrSw
                addRowDivider(card, yPos: r6 + rh)

                let r7 = 3 + rh * 7
                card.addSubview(makeLabel(tr("听歌自动戴耳机", "Auto Headphones on Music", "Fones automáticos com música"), yPos: r7, width: 360))
                let hpSw = makeSwitch(s.autoHeadphonesOnMusic, action: #selector(onToggleAutoHeadphones), yPos: r7)
                card.addSubview(hpSw); autoHeadphonesSwitch = hpSw
                addRowDivider(card, yPos: r7 + rh)

                let r8 = 3 + rh * 8
                card.addSubview(makeLabel("语言 / Language / Idioma", yPos: r8, width: 180))
                let lgSeg = NSSegmentedControl(labels: ["简体中文", "English", "Português"], trackingMode: .selectOne, target: self, action: #selector(onLangChanged))
                lgSeg.selectedSegment = Self.languageCodes.firstIndex(of: s.language) ?? 0
                lgSeg.frame = NSRect(x: cardW - 16 - 246, y: r8 + 8, width: 246, height: 24)
                card.addSubview(lgSeg); langSeg = lgSeg
            }
        }

        doc.frame = NSRect(x: 0, y: 0, width: winW, height: max(bodyH, y + 6))
        scroll.documentView = doc
        root.addSubview(scroll)

        // MARK: Clean Bottom Action Bar (no emoji)
        let footerLine = NSView(frame: NSRect(x: padX, y: winH - 50, width: cardW, height: 1))
        footerLine.wantsLayer = true
        footerLine.layer?.backgroundColor = KuroUITheme.divider.cgColor
        root.addSubview(footerLine)

        let batchBtnTitle = customCount > 0
            ? tr("批量删除皮肤 (\(customCount))", "Batch Delete (\(customCount))", "Excluir várias (\(customCount))")
            : tr("批量删除皮肤", "Batch Delete", "Excluir várias")
        let batchBtn = makeBtn(
            batchBtnTitle,
            variant: customCount > 0 ? .danger : .secondary,
            action: #selector(onTapSkinManager),
            frame: NSRect(x: padX, y: winH - 40, width: 148, height: 28),
            fontSize: 12
        )
        batchBtn.isEnabled = (customCount > 0)
        root.addSubview(batchBtn)

        let importFooterBtn = makeBtn(
            tr("导入皮肤…", "Import Skin…", "Importar skin…"),
            variant: .primary,
            action: #selector(onTapDressingRoom),
            frame: NSRect(x: winW - padX - 128, y: winH - 40, width: 128, height: 28),
            fontSize: 12
        )
        root.addSubview(importFooterBtn)

        win.contentView = root
    }



    @objc private func onSkinSelected() {
        guard let popup = skinPopup else { return }
        let idx = popup.indexOfSelectedItem
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
                eyePopup?.selectItem(at: eIdx)
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

    @objc private func onTapSkinManager() {
        onOpenSkinManager?()
    }

    @objc private func onKBSelected() {
        guard let popup = kbPopup else { return }
        let idx = popup.indexOfSelectedItem
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
        guard let popup = eyePopup else { return }
        let idx = popup.indexOfSelectedItem
        guard idx >= 0 && idx < EyeColorTheme.allCases.count else { return }
        CatSettings.shared.overrideSkinIris = true
        CatSettings.shared.eyeTheme = EyeColorTheme.allCases[idx]
        onSettingsChanged?()
        rebuildUI()
    }

    @objc private func onToggleWhiteOutline() {
        CatSettings.shared.whiteOutline = (whiteOutlineSwitch?.isOn == true)
        onSettingsChanged?()
    }

    @objc private func onToggleFlip() {
        CatSettings.shared.flipHorizontal = (flipSwitch?.isOn == true)
        UserDefaults.standard.synchronize()
        onSettingsChanged?()
    }

    @objc private func onRotationChanged() {
        guard let sl = rotSlider else { return }
        var deg = CGFloat(sl.doubleValue)
        for snap in [-180.0, -90.0, 0.0, 90.0, 180.0] as [CGFloat] {
            if abs(deg - snap) < 4.0 { deg = snap; break }
        }
        CatSettings.shared.rotationDeg = deg
        rotValLabel?.stringValue = "\(Int(deg.rounded()))°"
        onSettingsChanged?()
    }

    @objc private func onResetRotation() {
        CatSettings.shared.rotationDeg = 0
        rotSlider?.doubleValue = 0
        rotValLabel?.stringValue = "0°"
        UserDefaults.standard.synchronize()
        onSettingsChanged?()
    }

    @objc private func onScaleSliderChanged() {
        guard let sl = scaleSlider else { return }
        let sc = CGFloat(sl.doubleValue)
        scaleValLabel?.stringValue = "\(Int((sc * 100).rounded()))%"
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
        CatSettings.shared.enableDragAnimation = (dragAnimSwitch?.isOn == true)
        onSettingsChanged?()
    }

    @objc private func onToggleAnger() {
        CatSettings.shared.enableAnger = (angerSwitch?.isOn == true)
        onSettingsChanged?()
    }

    @objc private func onToggleBadges() {
        CatSettings.shared.showKeyBadges = (badgesSwitch?.isOn == true)
        onSettingsChanged?()
    }

    @objc private func onToggleMouseBadges() {
        CatSettings.shared.showMouseBadges = (mouseBadgesSwitch?.isOn == true)
        onSettingsChanged?()
    }

    @objc private func onTypingLimbSelected() {
        guard let popup = typingLimbPopup else { return }
        let idx = max(0, min(3, popup.indexOfSelectedItem))
        CatSettings.shared.footEasterEgg = (idx == 0 || idx == 1)
        CatSettings.shared.tailEasterEgg = (idx == 0 || idx == 2)
        onSettingsChanged?()
    }

    @objc private func onToggleLegends() {
        CatSettings.shared.showKeyLegends = (legendsSwitch?.isOn == true)
        onSettingsChanged?()
    }

    @objc private func onToggleMarquee() {
        CatSettings.shared.keyboardMarquee = (marqueeSwitch?.isOn == true)
        onSettingsChanged?()
    }

    @objc private func onToggleLevelKeyboard() {
        CatSettings.shared.levelKeyboard = (levelKeyboardSwitch?.isOn == true)
        onSettingsChanged?()
    }

    @objc private func onTabletSelected() {
        guard let popup = tabletPopup else { return }
        CatSettings.shared.tabletMode = max(0, min(2, popup.indexOfSelectedItem))
        onSettingsChanged?()
    }

    @objc private func onToggleFullscreenOverlay() {
        CatSettings.shared.fullScreenOverlay = (fullscreenOverlaySwitch?.isOn == true)
        onApplyWindowLevel?()
    }

    @objc private func onToggleClickThrough() {
        CatSettings.shared.fullClickThrough = (clickThroughSwitch?.isOn == true)
    }

    @objc private func onToggleAutoHide() {
        CatSettings.shared.autoHideFullscreen = (autoHideSwitch?.isOn == true)
        onAutoHideChanged?()
    }

    @objc private func onToggleAutoHeadphones() {
        CatSettings.shared.autoHeadphonesOnMusic = (autoHeadphonesSwitch?.isOn == true)
    }

    @objc private func onOBSSelected() {
        guard let popup = obsPopup else { return }
        let mode = max(0, min(2, popup.indexOfSelectedItem))
        CatSettings.shared.obsCaptureMode = mode
        if mode == 0 {
            CatSettings.shared.obsHideDesktop = false
            obsHideSwitch?.isOn = false
        }
        obsHideSwitch?.isEnabled = (mode > 0)
        onApplyOBSMode?()
    }

    @objc private func onToggleOBSHide() {
        CatSettings.shared.obsHideDesktop = (obsHideSwitch?.isOn == true)
        onApplyOBSMode?()
    }

    @objc private func onToggleAutoStart() {
        setAutoStart?(autoStartSwitch?.isOn == true)
    }

    @objc private func onToggleHourlyAnnouncement() {
        CatSettings.shared.hourlyAnnouncement = (hourlySwitch?.isOn == true)
    }

    /// Segment order of the language picker.
    private static let languageCodes = ["zh", "en", "pt"]

    @objc private func onLangChanged() {
        guard let seg = langSeg else { return }
        CatSettings.shared.language = Self.languageCodes[max(0, min(seg.selectedSegment, Self.languageCodes.count - 1))]
        onLanguageChanged?()
        rebuildUI()
    }
}

// MARK: - Batch Delete Imported Skins Window (Lightweight Vector Checklist, Releases Views on Close)

private final class SkinSwatchDotView: NSView {
    private let coatColor: NSColor
    private let eyeColor: NSColor?
    override var isFlipped: Bool { true }

    init(frame: NSRect, coatColor: NSColor, eyeColor: NSColor?) {
        self.coatColor = coatColor
        self.eyeColor = eyeColor
        super.init(frame: frame)
    }
    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }

    override func draw(_ dirtyRect: NSRect) {
        let circle = NSBezierPath(ovalIn: bounds.insetBy(dx: 0.5, dy: 0.5))
        coatColor.setFill()
        circle.fill()
        KuroUITheme.border.setStroke()
        circle.lineWidth = 1
        circle.stroke()
        if let eyeColor {
            let dot = NSBezierPath(ovalIn: NSRect(x: bounds.midX - 3, y: bounds.midY - 3, width: 6, height: 6))
            eyeColor.setFill()
            dot.fill()
        }
    }
}

private final class SkinBatchRowView: NSView {
    var onToggle: (() -> Void)?
    var isSelectedRow: Bool = false { didSet { needsDisplay = true } }
    override var isFlipped: Bool { true }

    override func resetCursorRects() {
        addCursorRect(bounds, cursor: .pointingHand)
    }

    override func mouseDown(with event: NSEvent) {
        onToggle?()
    }

    override func draw(_ dirtyRect: NSRect) {
        let rect = bounds.insetBy(dx: 0.5, dy: 0.5)
        let path = NSBezierPath(roundedRect: rect, xRadius: 10, yRadius: 10)
        (isSelectedRow ? KuroUITheme.accentSoft : KuroUITheme.cardBg).setFill()
        path.fill()
        (isSelectedRow ? KuroUITheme.accent.withAlphaComponent(0.55) : KuroUITheme.border).setStroke()
        path.lineWidth = 1
        path.stroke()
    }
}

/// Focused checklist window for batch-deleting `.kurocat` skins without holding thumbnail bitmaps in memory.
final class SkinBatchDeleteWindowController: NSWindowController, NSWindowDelegate, NSSearchFieldDelegate {
    var onBatchDelete: (([String]) -> Bool)?

    private var selectedIds: Set<String> = []
    private var query: String = ""
    private var searchField: NSSearchField?
    private var listDoc: FlippedView?
    private var countLabel: NSTextField?
    private var deleteBtn: KuroThemedButton?

    init() {
        let win = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 460, height: 520),
            styleMask: [.titled, .closable],
            backing: .buffered,
            defer: false
        )
        win.isReleasedWhenClosed = false
        win.level = .floating
        win.titlebarAppearsTransparent = true
        win.isMovableByWindowBackground = true
        win.backgroundColor = KuroUITheme.windowBg
        super.init(window: win)
        win.delegate = self
        win.center()
    }

    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }

    private func tr(_ zh: String, _ en: String, _ pt: String) -> String {
        loc(zh, en, pt)
    }

    func windowWillClose(_ notification: Notification) {
        selectedIds.removeAll()
        query = ""
        searchField = nil
        listDoc = nil
        countLabel = nil
        deleteBtn = nil
        window?.contentView = nil
    }

    func syncWindowLevel() {
        guard let window else { return }
        window.level = CatSettings.shared.fullScreenOverlay
            ? NSWindow.Level(rawValue: Int(CGWindowLevelForKey(.popUpMenuWindow)) + 1)
            : .floating
        window.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]
    }

    func present() {
        syncWindowLevel()
        selectedIds.removeAll()
        query = ""
        rebuildUI()
        NSApp.activate(ignoringOtherApps: true)
        showWindow(nil)
        window?.makeKeyAndOrderFront(nil)
        window?.orderFrontRegardless()
    }

    func refresh() {
        guard window?.isVisible == true else { return }
        let validIds = Set(SkinLibrary.shared.skins.map { $0.id })
        selectedIds = selectedIds.intersection(validIds)
        rebuildListRows()
    }

    private func filteredSkins() -> [SkinSpec] {
        let all = SkinLibrary.shared.skins
        let q = query.trimmingCharacters(in: .whitespacesAndNewlines).lowercased()
        guard !q.isEmpty else { return all }
        return all.filter {
            $0.name.lowercased().contains(q)
                || $0.author.lowercased().contains(q)
                || $0.id.lowercased().contains(q)
        }
    }

    func controlTextDidChange(_ obj: Notification) {
        query = searchField?.stringValue ?? ""
        rebuildListRows()
    }

    private func makeBtn(_ title: String, variant: KuroThemedButton.Variant, action: Selector, frame: NSRect) -> KuroThemedButton {
        let btn = KuroThemedButton(title: title, target: self, action: action)
        btn.isBordered = false
        btn.variant = variant
        btn.fontSize = 11.5
        btn.frame = frame
        return btn
    }

    private func rebuildUI() {
        guard let win = window else { return }
        win.title = tr("批量删除皮肤", "Batch Delete Skins", "Excluir várias skins")
        win.backgroundColor = KuroUITheme.windowBg

        let winW: CGFloat = 460
        let winH: CGFloat = 520
        let padX: CGFloat = 18
        let innerW = winW - padX * 2
        let root = FlippedView(frame: NSRect(x: 0, y: 0, width: winW, height: winH))

        let titleLbl = NSTextField(labelWithString: tr("批量管理与清理皮肤", "Batch Delete Imported Skins", "Gerenciar skins importadas"))
        titleLbl.font = .systemFont(ofSize: 17, weight: .bold)
        titleLbl.textColor = KuroUITheme.ink
        titleLbl.frame = NSRect(x: padX, y: 12, width: innerW, height: 22)
        root.addSubview(titleLbl)

        let subLbl = NSTextField(labelWithString: tr("勾选不需要的 .kurocat 文件一次删除；内置 4 款猫咪不受影响", "Check unwanted .kurocat files to remove at once; built-in coats stay safe", "Marque os arquivos .kurocat que não quer mais para excluir de uma vez; as pelagens nativas não são afetadas"))
        subLbl.font = .systemFont(ofSize: 11.5)
        subLbl.textColor = KuroUITheme.secondaryInk
        subLbl.frame = NSRect(x: padX, y: 36, width: innerW, height: 16)
        root.addSubview(subLbl)

        let sf = NSSearchField(frame: NSRect(x: padX, y: 60, width: innerW, height: 26))
        sf.placeholderString = tr("搜索皮肤名称或作者…", "Search name or author…", "Buscar por nome ou autor…")
        sf.stringValue = query
        sf.delegate = self
        root.addSubview(sf)
        searchField = sf

        let barY: CGFloat = 94
        root.addSubview(makeBtn(tr("全选", "All", "Todas"), variant: .secondary, action: #selector(onSelectAll), frame: NSRect(x: padX, y: barY, width: 54, height: 24)))
        root.addSubview(makeBtn(tr("清空", "None", "Nenhuma"), variant: .secondary, action: #selector(onSelectNone), frame: NSRect(x: padX + 60, y: barY, width: 54, height: 24)))
        root.addSubview(makeBtn(tr("反选", "Invert", "Inverter"), variant: .secondary, action: #selector(onInvertSelection), frame: NSRect(x: padX + 120, y: barY, width: 54, height: 24)))
        root.addSubview(makeBtn(tr("仅选未穿戴", "Unused Only", "Só as não usadas"), variant: .secondary, action: #selector(onSelectUnused), frame: NSRect(x: padX + 180, y: barY, width: 88, height: 24)))

        let cntLbl = NSTextField(labelWithString: "")
        cntLbl.font = .monospacedDigitSystemFont(ofSize: 11.5, weight: .semibold)
        cntLbl.textColor = KuroUITheme.secondaryInk
        cntLbl.alignment = .right
        cntLbl.frame = NSRect(x: winW - padX - 130, y: barY + 4, width: 130, height: 17)
        root.addSubview(cntLbl)
        countLabel = cntLbl

        let listY: CGFloat = 126
        let listH: CGFloat = winH - listY - 58
        let scroll = NSScrollView(frame: NSRect(x: padX, y: listY, width: innerW, height: listH))
        scroll.hasVerticalScroller = true
        scroll.drawsBackground = false
        scroll.borderType = .noBorder

        let doc = FlippedView(frame: NSRect(x: 0, y: 0, width: innerW, height: listH))
        scroll.documentView = doc
        listDoc = doc
        root.addSubview(scroll)

        let footDiv = NSView(frame: NSRect(x: padX, y: winH - 50, width: innerW, height: 1))
        footDiv.wantsLayer = true
        footDiv.layer?.backgroundColor = KuroUITheme.divider.cgColor
        root.addSubview(footDiv)

        root.addSubview(makeBtn(tr("关闭", "Close", "Fechar"), variant: .secondary, action: #selector(onClose), frame: NSRect(x: padX, y: winH - 40, width: 84, height: 28)))
        let delBtn = makeBtn(tr("删除所选", "Delete Selected", "Excluir selecionadas"), variant: .dangerSolid, action: #selector(onConfirmDelete), frame: NSRect(x: winW - padX - 148, y: winH - 40, width: 148, height: 28))
        root.addSubview(delBtn)
        deleteBtn = delBtn

        win.contentView = root
        rebuildListRows()
    }

    private func rebuildListRows() {
        guard let doc = listDoc else { return }
        doc.subviews.forEach { $0.removeFromSuperview() }

        let skins = filteredSkins()
        let total = SkinLibrary.shared.skins.count
        let innerW = doc.frame.width
        let activeId = CatSettings.shared.customSkin

        if skins.isEmpty {
            let emptyMsg = total == 0
                ? tr("暂无已导入的自定义皮肤", "No imported custom skins yet", "Nenhuma skin importada ainda")
                : tr("没有匹配当前搜索词的皮肤", "No skins match your search", "Nenhuma skin corresponde à busca")
            let lbl = NSTextField(labelWithString: emptyMsg)
            lbl.alignment = .center
            lbl.font = .systemFont(ofSize: 13)
            lbl.textColor = KuroUITheme.secondaryInk
            lbl.frame = NSRect(x: 12, y: 90, width: innerW - 24, height: 22)
            doc.addSubview(lbl)
            doc.frame = NSRect(x: 0, y: 0, width: innerW, height: 220)
        } else {
            let rowH: CGFloat = 46
            let gap: CGFloat = 6
            for (idx, sk) in skins.enumerated() {
                let ry = CGFloat(idx) * (rowH + gap)
                let isSel = selectedIds.contains(sk.id)
                let row = SkinBatchRowView(frame: NSRect(x: 0, y: ry, width: innerW, height: rowH))
                row.isSelectedRow = isSel
                let skinId = sk.id
                row.onToggle = { [weak self] in
                    guard let self else { return }
                    if self.selectedIds.contains(skinId) {
                        self.selectedIds.remove(skinId)
                    } else {
                        self.selectedIds.insert(skinId)
                    }
                    self.rebuildListRows()
                }

                let check = NSButton(checkboxWithTitle: "", target: self, action: #selector(onRowCheck(_:)))
                check.state = isSel ? .on : .off
                check.identifier = NSUserInterfaceItemIdentifier(skinId)
                check.frame = NSRect(x: 10, y: 13, width: 20, height: 20)
                row.addSubview(check)

                let coatCol: NSColor
                if sk.renderStyle == .beagle {
                    coatCol = NSColor(srgbRed: 0.78, green: 0.56, blue: 0.37, alpha: 1)
                } else if sk.renderStyle == .douni {
                    coatCol = NSColor(srgbRed: 0.96, green: 0.95, blue: 0.93, alpha: 1)
                } else {
                    coatCol = sk.palette.body.srgbNS
                }
                let swatch = SkinSwatchDotView(frame: NSRect(x: 34, y: 14, width: 18, height: 18), coatColor: coatCol, eyeColor: sk.palette.iris?.srgbNS)
                row.addSubview(swatch)

                let isWearing = (sk.id == activeId)
                let nameText = SkinLibrary.shared.displayName(for: sk)
                let nameLbl = NSTextField(labelWithString: nameText)
                nameLbl.font = .systemFont(ofSize: 13, weight: .semibold)
                nameLbl.textColor = KuroUITheme.ink
                nameLbl.lineBreakMode = .byTruncatingTail
                nameLbl.frame = NSRect(x: 60, y: 6, width: innerW - (isWearing ? 128 : 74), height: 18)
                row.addSubview(nameLbl)

                if isWearing {
                    let pill = KuroBadgePillView(
                        frame: NSRect(x: innerW - 64, y: 7, width: 52, height: 17),
                        text: tr("正在穿", "Active", "Em uso"),
                        bg: KuroUITheme.accent,
                        fg: KuroUITheme.accentForeground
                    )
                    row.addSubview(pill)
                }

                let styleTag: String = {
                    switch sk.renderStyle {
                    case .douni, .beagle: return tr("彩蛋款", "Easter Egg", "Easter egg")
                    case .dazed: return tr("呆猫款", "Dazed", "Atordoado")
                    case .standard: return tr("标准款", "Custom", "Padrão")
                    }
                }()
                let authorPart = sk.author.isEmpty ? tr("无署名", "Anonymous", "Sem autor") : sk.author
                let sub = NSTextField(labelWithString: "\(authorPart) · \(styleTag)")
                sub.font = .systemFont(ofSize: 11)
                sub.textColor = KuroUITheme.secondaryInk
                sub.lineBreakMode = .byTruncatingTail
                sub.frame = NSRect(x: 60, y: 25, width: innerW - 74, height: 15)
                row.addSubview(sub)

                doc.addSubview(row)
            }
            let totalH = CGFloat(skins.count) * (rowH + gap)
            doc.frame = NSRect(x: 0, y: 0, width: innerW, height: max(220, totalH))
        }

        let selCount = selectedIds.count
        countLabel?.stringValue = tr("已选 \(selCount) / \(total)", "\(selCount) / \(total) selected", "\(selCount) / \(total) selecionada(s)")
        deleteBtn?.title = selCount > 0
            ? tr("删除所选 (\(selCount))", "Delete (\(selCount))", "Excluir (\(selCount))")
            : tr("删除所选", "Delete Selected", "Excluir selecionadas")
        deleteBtn?.isEnabled = (selCount > 0)
    }

    @objc private func onRowCheck(_ sender: NSButton) {
        guard let id = sender.identifier?.rawValue else { return }
        if sender.state == .on {
            selectedIds.insert(id)
        } else {
            selectedIds.remove(id)
        }
        rebuildListRows()
    }

    @objc private func onSelectAll() {
        for sk in filteredSkins() { selectedIds.insert(sk.id) }
        rebuildListRows()
    }

    @objc private func onSelectNone() {
        let visible = Set(filteredSkins().map { $0.id })
        selectedIds.subtract(visible)
        rebuildListRows()
    }

    @objc private func onInvertSelection() {
        for sk in filteredSkins() {
            if selectedIds.contains(sk.id) { selectedIds.remove(sk.id) } else { selectedIds.insert(sk.id) }
        }
        rebuildListRows()
    }

    @objc private func onSelectUnused() {
        let activeId = CatSettings.shared.customSkin
        for sk in filteredSkins() where sk.id != activeId {
            selectedIds.insert(sk.id)
        }
        selectedIds.remove(activeId)
        rebuildListRows()
    }

    @objc private func onClose() {
        window?.close()
    }

    @objc private func onConfirmDelete() {
        let ordered = SkinLibrary.shared.skins.map(\.id).filter { selectedIds.contains($0) }
        guard !ordered.isEmpty else { return }
        _ = onBatchDelete?(ordered)
    }
}
