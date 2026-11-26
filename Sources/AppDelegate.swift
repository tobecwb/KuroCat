import Foundation
import AppKit
import CoreAudio

final class AppDelegate: NSObject, NSApplicationDelegate, NSMenuDelegate {
    private enum RenderCadence {
        case paused
        case fullscreenHidden // 2 Hz check while hidden behind a fullscreen app
        case asleep           // 15 Hz adaptive cadence while sleeping
        case idle             // 30 Hz adaptive cadence while awake at rest
        case active           // 60 Hz full cadence during input or motion

        var interval: TimeInterval {
            switch self {
            case .paused:           return 0
            case .fullscreenHidden: return 0.5
            case .asleep:           return 1.0 / 15.0
            case .idle:             return 1.0 / 30.0
            case .active:           return 1.0 / 60.0
            }
        }

        var tolerance: TimeInterval {
            switch self {
            case .paused:           return 0
            case .fullscreenHidden: return 0.15
            case .asleep:           return 0.012
            case .idle:             return 0.004
            case .active:           return 0.002
            }
        }
    }

    private var statusItem: NSStatusItem!
    private var overlayWindow: OverlayWindow!
    private var renderTimer: Timer?
    private var renderCadence: RenderCadence = .paused
    private var windowWatchdogTimer: Timer?
    private var lastFrameTime: CFTimeInterval = CACurrentMediaTime()
    private var lastMusicCheckTime: CFTimeInterval = 0
    private var musicAudioSustainedSince: CFTimeInterval = 0
    private var cachedMusicPlaying: Bool = false
    private let musicCheckQueue = DispatchQueue(label: "com.kurocat.musiccheck", qos: .utility)
    private var lastFullscreenCheckTime: CFTimeInterval = 0
    private var hiddenByFullscreen = false
    private var screensAsleep = false
    private var lastDemoKeyTime: CFTimeInterval = 0
    private var isPetVisible = true
    private var availableUpdateVersion: String?
    private var availableUpdateURL: URL?
    private var availableAssetURL: URL?
    private var isUpdating = false
    private var isHandingOverToNewerCopy = false
    private var isPresentingModalAlert = false
    private var dressingRoom: DressingRoomWindowController?
    private var settingsWindow: SettingsWindowController?
    private var skinManagerWindow: SkinBatchDeleteWindowController?
    private var updatePromptController: UpdatePromptWindowController?

    // MARK: - Hourly Announcement
    private var minuteTimer: Timer?
    private var lastAnnouncedHour: Int = -1

    private var currentAppVersion: String {
        Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String ?? "1.4.7"
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.setActivationPolicy(.accessory)
        if resolveDuplicateRunningCopies() { return }

        overlayWindow = OverlayWindow()
        overlayWindow.orderFrontRegardless()

        let catView = overlayWindow.catView
        catView.onRequestPermission = { [weak self] in
            self?.openInputMonitoringSettings()
        }
        catView.onRightClickMenu = { [weak self] event in
            guard let self else { return }
            self.wakeToActive()
            let menu = self.buildMenu()
            NSMenu.popUpContextMenu(menu, with: event, for: self.overlayWindow.catView)
        }
        catView.onUserInteraction = { [weak self] in
            self?.wakeToActive()
        }
        catView.onDropSkinFiles = { [weak self] urls in
            for url in urls { self?.importDroppedFile(from: url) }
        }

        // Wire dual-layer InputEngine -> CatPhysics
        let engine = InputEngine.shared
        engine.onKeyDown = { [weak self] key in
            DispatchQueue.main.async {
                guard let self else { return }
                self.overlayWindow.catView.physics.triggerKeyDown(key)
                self.wakeToActive()
            }
        }
        engine.onMouseClick = { [weak self] button, isDown in
            DispatchQueue.main.async {
                guard let self else { return }
                self.overlayWindow.catView.physics.triggerMouseClick(button: button, isDown: isDown)
                self.wakeToActive()
            }
        }
        engine.start()

        setupStatusItem()
        if isAutoStartEnabled() {
            // Keep LaunchAgent path updated if user moved KuroCat.app into /Applications
            _ = setAutoStartEnabled(true)
        }
        observePowerNotifications()
        startRenderLoop()
        startWindowWatchdog()
        startHourlyAnnouncement()

    }

    func applicationWillTerminate(_ notification: Notification) {
        // Quitting during launch because a newer copy is running: leave its stats alone.
        if isHandingOverToNewerCopy { return }
        renderTimer?.invalidate()
        renderTimer = nil
        renderCadence = .paused
        windowWatchdogTimer?.invalidate()
        minuteTimer?.invalidate()
        NSWorkspace.shared.notificationCenter.removeObserver(self)
        InputEngine.shared.flushStats()
        InputEngine.shared.stop()
    }

    // MARK: - One KuroCat at a time (newest copy wins)

    /// Users often download a new KuroCat.app while an older copy is still running (or is started
    /// by the login item from another folder). That old process kept running and kept announcing
    /// "new version available" even though they had already updated. Keep only the newest copy:
    /// quit older ones, or hand over to an already-running newer one. Returns true if THIS process quits.
    private func resolveDuplicateRunningCopies() -> Bool {
        guard let bundleID = Bundle.main.bundleIdentifier else { return false }
        let myPID = ProcessInfo.processInfo.processIdentifier
        let myURL = Bundle.main.bundleURL.standardizedFileURL
        let others = NSRunningApplication.runningApplications(withBundleIdentifier: bundleID)
            .filter { $0.processIdentifier != myPID }
        if others.isEmpty { return false }

        for other in others {
            guard let otherURL = other.bundleURL?.standardizedFileURL, otherURL != myURL,
                  let otherVersion = Self.shortVersion(ofAppAt: otherURL) else { continue }
            if isVersion(otherVersion, newerThan: currentAppVersion) {
                isHandingOverToNewerCopy = true
                _ = other.activate(options: [])
                NSApp.terminate(nil)
                return true
            }
        }
        for other in others {
            // Same path (replaced in place by an update) or an older/equal copy elsewhere.
            if !other.terminate() { _ = other.forceTerminate() }
            DispatchQueue.main.asyncAfter(deadline: .now() + 3.0) {
                if !other.isTerminated { _ = other.forceTerminate() }
            }
        }
        return false
    }

    /// Reads CFBundleShortVersionString straight from disk (Bundle(url:) may return cached info).
    private static func shortVersion(ofAppAt url: URL) -> String? {
        let plistURL = url.appendingPathComponent("Contents/Info.plist")
        guard let dict = NSDictionary(contentsOf: plistURL) as? [String: Any] else { return nil }
        return dict["CFBundleShortVersionString"] as? String
    }

    /// A newer KuroCat.app that is already on disk (e.g. downloaded to ~/Downloads but never
    /// launched because LaunchServices just re-activated this running copy).
    private func newestDownloadedCopy() -> (url: URL, version: String)? {
        guard let bundleID = Bundle.main.bundleIdentifier else { return nil }
        let myURL = Bundle.main.bundleURL.standardizedFileURL
        var best: (url: URL, version: String)?
        let appURLs: [URL]
        if #available(macOS 12.0, *) {
            appURLs = NSWorkspace.shared.urlsForApplications(withBundleIdentifier: bundleID)
        } else {
            appURLs = (LSCopyApplicationURLsForBundleIdentifier(bundleID as CFString, nil)?.takeRetainedValue() as? [URL]) ?? []
        }
        for url in appURLs {
            let std = url.standardizedFileURL
            guard std != myURL, !std.path.contains("/.Trash/"),
                  let v = Self.shortVersion(ofAppAt: std),
                  isVersion(v, newerThan: best?.version ?? currentAppVersion) else { continue }
            best = (std, v)
        }
        return best
    }

    private func switchToDownloadedCopy(_ url: URL) {
        let config = NSWorkspace.OpenConfiguration()
        config.createsNewApplicationInstance = true
        NSWorkspace.shared.openApplication(at: url, configuration: config) { _, error in
            DispatchQueue.main.async {
                if error == nil { NSApp.terminate(nil) }
            }
        }
    }

    // MARK: - Adaptive Redraw

    private func observePowerNotifications() {
        let nc = NSWorkspace.shared.notificationCenter
        nc.addObserver(self, selector: #selector(handleScreenOrSystemSleep),
                       name: NSWorkspace.screensDidSleepNotification, object: nil)
        nc.addObserver(self, selector: #selector(handleScreenOrSystemWake),
                       name: NSWorkspace.screensDidWakeNotification, object: nil)
        nc.addObserver(self, selector: #selector(handleScreenOrSystemSleep),
                       name: NSWorkspace.willSleepNotification, object: nil)
        nc.addObserver(self, selector: #selector(handleScreenOrSystemWake),
                       name: NSWorkspace.didWakeNotification, object: nil)
    }

    @objc private func handleScreenOrSystemSleep() {
        screensAsleep = true
        configureRenderTimer(for: .paused)
    }

    @objc private func handleScreenOrSystemWake() {
        screensAsleep = false
        lastFrameTime = CACurrentMediaTime()
        wakeToActive(immediateRedraw: true)
    }

    private func startRenderLoop() {
        lastFrameTime = CACurrentMediaTime()
        wakeToActive()
    }

    private func wakeToActive(immediateRedraw: Bool = false) {
        guard isPetVisible, !screensAsleep else { return }
        if !hiddenByFullscreen && renderCadence != .active {
            configureRenderTimer(for: .active)
        }
        if immediateRedraw && !hiddenByFullscreen {
            overlayWindow?.catView.needsDisplay = true
        }
    }

    private func configureRenderTimer(for cadence: RenderCadence) {
        guard cadence != renderCadence || (cadence != .paused && renderTimer == nil) else { return }
        renderTimer?.invalidate()
        renderTimer = nil
        renderCadence = cadence
        guard cadence != .paused else { return }
        let timer = Timer(timeInterval: cadence.interval, repeats: true) { [weak self] _ in
            self?.renderTick()
        }
        timer.tolerance = cadence.tolerance
        RunLoop.main.add(timer, forMode: .common)
        renderTimer = timer
    }

    private let fullscreenCheckQueue = DispatchQueue(label: "com.kurocat.fullscreen-check", qos: .utility)
    private var fullscreenCheckInFlight = false

    private func updateFullscreenAutoHide(now: CFTimeInterval) -> Bool {
        let s = CatSettings.shared
        let obsBg = (s.obsCaptureMode > 0 && s.obsHideDesktop)
        if !s.autoHideFullscreen || obsBg {
            if hiddenByFullscreen {
                hiddenByFullscreen = false
                overlayWindow.alphaValue = 1.0
                lastFrameTime = now
                overlayWindow.catView.needsDisplay = true
            }
            return false
        }
        if now - lastFullscreenCheckTime <= 0.5 || fullscreenCheckInFlight {
            return hiddenByFullscreen
        }
        lastFullscreenCheckTime = now
        fullscreenCheckInFlight = true
        fullscreenCheckQueue.async { [weak self] in
            let fs = Self.isOtherAppFullscreen()
            DispatchQueue.main.async { [weak self] in
                guard let self else { return }
                self.fullscreenCheckInFlight = false
                let s = CatSettings.shared
                let obsBg = (s.obsCaptureMode > 0 && s.obsHideDesktop)
                guard s.autoHideFullscreen && !obsBg else { return }
                if fs != self.hiddenByFullscreen {
                    self.hiddenByFullscreen = fs
                    self.overlayWindow.alphaValue = fs ? 0.0 : 1.0
                    if fs {
                        self.overlayWindow.ignoresMouseEvents = true
                    } else {
                        self.lastFrameTime = CACurrentMediaTime()
                        self.overlayWindow.catView.needsDisplay = true
                    }
                }
            }
        }
        return hiddenByFullscreen
    }

    /// Returns true if any active macOS audio output device (speakers, AirPods/Bluetooth/USB headphones,
    /// multi-output or virtual device) is currently streaming audio. Works for Apple Music, browsers
    /// (Bilibili/YouTube/Douyin/etc.), and all local video/music players without needing an app whitelist.
    private static func isAnyAudioOutputRunning() -> Bool {
        func deviceHasOutputAndRunning(_ devID: AudioDeviceID) -> Bool {
            var sSize: UInt32 = 0
            var sAddr = AudioObjectPropertyAddress(
                mSelector: kAudioDevicePropertyStreamConfiguration,
                mScope: kAudioObjectPropertyScopeOutput,
                mElement: kAudioObjectPropertyElementMain
            )
            guard AudioObjectGetPropertyDataSize(devID, &sAddr, 0, nil, &sSize) == noErr, sSize > 0 else {
                return false
            }
            let buf = UnsafeMutableRawPointer.allocate(
                byteCount: Int(sSize),
                alignment: MemoryLayout<AudioBufferList>.alignment
            )
            defer { buf.deallocate() }
            var outChannels: UInt32 = 0
            if AudioObjectGetPropertyData(devID, &sAddr, 0, nil, &sSize, buf) == noErr {
                let abl = UnsafeMutableAudioBufferListPointer(buf.assumingMemoryBound(to: AudioBufferList.self))
                for b in abl { outChannels += b.mNumberChannels }
            }
            guard outChannels > 0 else { return false }

            var running: UInt32 = 0
            var runSize = UInt32(MemoryLayout<UInt32>.size)
            var runAddr = AudioObjectPropertyAddress(
                mSelector: kAudioDevicePropertyDeviceIsRunningSomewhere,
                mScope: kAudioObjectPropertyScopeGlobal,
                mElement: kAudioObjectPropertyElementMain
            )
            guard AudioObjectGetPropertyData(devID, &runAddr, 0, nil, &runSize, &running) == noErr else {
                return false
            }
            return running != 0
        }

        // Enumerate all audio devices so Bluetooth/USB headphones & secondary output devices are always covered
        var propSize: UInt32 = 0
        var listAddr = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDevices,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain
        )
        if AudioObjectGetPropertyDataSize(AudioObjectID(kAudioObjectSystemObject), &listAddr, 0, nil, &propSize) == noErr,
           propSize >= UInt32(MemoryLayout<AudioDeviceID>.size) {
            let count = Int(propSize) / MemoryLayout<AudioDeviceID>.size
            var devices = [AudioDeviceID](repeating: AudioDeviceID(kAudioObjectUnknown), count: count)
            if AudioObjectGetPropertyData(AudioObjectID(kAudioObjectSystemObject), &listAddr, 0, nil, &propSize, &devices) == noErr {
                for dev in devices where dev != kAudioObjectUnknown {
                    if deviceHasOutputAndRunning(dev) { return true }
                }
                return false
            }
        }

        // Fallback: check default output device directly
        var devID = AudioObjectID(kAudioObjectUnknown)
        var devSize = UInt32(MemoryLayout<AudioObjectID>.size)
        var devAddr = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDefaultOutputDevice,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain
        )
        guard AudioObjectGetPropertyData(AudioObjectID(kAudioObjectSystemObject), &devAddr, 0, nil, &devSize, &devID) == noErr,
              devID != kAudioObjectUnknown else {
            return false
        }
        return deviceHasOutputAndRunning(devID)
    }

    private func pollMusicStateIfNeeded(now: CFTimeInterval) {
        guard CatSettings.shared.autoHeadphonesOnMusic else {
            if cachedMusicPlaying {
                cachedMusicPlaying = false
                musicAudioSustainedSince = 0
                overlayWindow.catView.physics.isMusicPlaying = false
            }
            return
        }
        if now - lastMusicCheckTime < 1.0 { return }
        lastMusicCheckTime = now

        musicCheckQueue.async { [weak self] in
            guard let self = self else { return }
            let audioRunning = Self.isAnyAudioOutputRunning()
            DispatchQueue.main.async {
                let playing: Bool
                if audioRunning {
                    if self.musicAudioSustainedSince == 0 {
                        self.musicAudioSustainedSince = now
                    }
                    // Require >= 1.5s continuous audio output to ignore brief UI/notification dings
                    playing = (now - self.musicAudioSustainedSince) >= 1.5
                } else {
                    self.musicAudioSustainedSince = 0
                    playing = false
                }
                self.cachedMusicPlaying = playing
                self.overlayWindow.catView.physics.isMusicPlaying = playing
            }
        }
    }

    private func renderTick() {
        guard isPetVisible, !screensAsleep else {
            configureRenderTimer(for: .paused)
            return
        }
        let now = CACurrentMediaTime()
        if updateFullscreenAutoHide(now: now) {
            configureRenderTimer(for: .fullscreenHidden)
            return
        }
        pollMusicStateIfNeeded(now: now)

        let dt = CGFloat(now - lastFrameTime)
        lastFrameTime = now

        let view = overlayWindow.catView
        let physics = view.physics
        let input = InputEngine.shared
        let windowFrame = overlayWindow.frame
        let physicsFrame = overlayWindow.physicsFrameInScreen
        let previousMouseMoveTime = input.lastMouseMoveTime
        let wasBlinking = physics.isBlinking
        let wasYawning = physics.isYawning
        let wasToastActive = view.toastActive
        physics.step(dt: dt, windowFrameInScreen: physicsFrame)
        view.updateHoverAndClickThrough()
        view.tickToast(dt: dt)

        let lastDirectInput = max(input.lastKeyActivityTime, input.lastMouseClickTime, lastDemoKeyTime)
        let mouseMoving = now - input.lastMouseMoveTime < 0.12
        let mouseStartedMoving = mouseMoving && now - previousMouseMoveTime >= 0.12
        let mouseNearPet = mouseMoving && windowFrame.insetBy(dx: -24, dy: -24).contains(input.lastMouseLocation)
        let isAsleep = physics.sleepiness >= 0.95
        let hasActiveSparkles = physics.sparkles.contains { !$0.isSleepZ }
        let hasMotion = hasActiveSparkles || !physics.floatingKeycaps.isEmpty
            || !physics.keyGlow.isEmpty || !physics.tabletTrail.isEmpty
            || physics.kbSparkTimer > 0.005
            || physics.petHeadIntensity > 0.02 || physics.petBodyIntensity > 0.02
            || physics.huntIntensity.value > 0.02
            || physics.scruffDrag.value > 0.02 || physics.isChangingLie
            || abs(physics.scruffSwing.value) > 0.01 || abs(physics.scruffSwingY.value) > 0.01
            || abs(physics.scruffTailSwing.value) > 0.01
            || (physics.douniLoafBlend.value > 0.005 && physics.douniLoafBlend.value < 0.995)
            || abs(physics.douniLoafBlend.velocity) > 0.02
            || abs(physics.douniLeftRaise.velocity) > 0.05 || abs(physics.douniRightRaise.velocity) > 0.05
            || abs(physics.douniMouseBlend.velocity) > 0.05
            || abs(physics.kbImpactY.value) > 0.01 || abs(physics.kbImpactTilt.value) > 0.01
            || physics.angerIntensity.value > 0.02 || physics.dizzyIntensity.value > 0.02
            || physics.scruffDizzyIntensity.value > 0.02 || physics.mousePressMiddle.value > 0.02
            || wasYawning || physics.isYawning
            || (!isAsleep && (wasBlinking || physics.isBlinking))
        let isActive = now - lastDirectInput < 0.8 || !input.activeKeys.isEmpty
            || mouseStartedMoving || mouseNearPet
            || input.leftMouseDown || input.rightMouseDown || input.otherMouseDown
            || view.isDraggingOrResizing || hasMotion
            || wasToastActive || view.toastActive  // hourly toast animates at full rate

        let desiredCadence: RenderCadence = isActive ? .active : (isAsleep ? .asleep : .idle)
        if desiredCadence != renderCadence {
            if desiredCadence == .asleep {
                CatRenderer.purgeVolatileCaches()
            }
            configureRenderTimer(for: desiredCadence)
        }
        if isActive || view.poseNeedsDisplay() {
            view.needsDisplay = true
        }
    }

    // MARK: - Menu Bar Icon & Menu

    private func setupStatusItem() {
        statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)
        if let button = statusItem.button {
            button.image = makeMenuBarCatIcon()
            button.toolTip = loc("KuroCat 黑猫键盘桌宠", "KuroCat — keyboard cat desktop pet", "KuroCat — gatinho de teclado para a área de trabalho")
        }
        let menu = NSMenu()
        menu.delegate = self
        statusItem.menu = menu
    }

    func menuNeedsUpdate(_ menu: NSMenu) {
        menu.removeAllItems()
        for item in buildMenu().items {
            item.menu?.removeItem(item)
            menu.addItem(item)
        }
    }

    private func buildMenu() -> NSMenu {
        let menu = NSMenu()
        let engine = InputEngine.shared
        let settings = CatSettings.shared
        let physics = overlayWindow.catView.physics

        func tr(_ zh: String, _ eng: String, _ pt: String) -> String { loc(zh, eng, pt) }

        // Status header
        let statusPrefix = engine.status.needsAttention ? "⚠️" : "🟢"
        let statusText: String
        switch engine.status {
        case .fullHardware: statusText = tr(engine.status.rawValue, "Hardware Input Active (HID + EventTap)", "Captura por hardware ativa (HID + EventTap)")
        case .hidOnly: statusText = tr(engine.status.rawValue, "Hardware HID Active", "Captura HID ativa (jogos e digitação)")
        case .eventTapOnly: statusText = tr(engine.status.rawValue, "Global EventTap Active", "Captura global ativa (EventTap)")
        case .needsPermission: statusText = tr(engine.status.rawValue, "Input Monitoring Permission Needed", "Precisa da permissão de Monitoramento de Entrada")
        case .needsRestart: statusText = tr(engine.status.rawValue, "Authorized — Restart to Retry", "Autorizado — reinicie para tentar de novo")
        }
        let statusItem = NSMenuItem(title: "\(statusPrefix) \(statusText)", action: engine.status.needsAttention ? #selector(openInputMonitoringSettings) : nil, keyEquivalent: "")
        statusItem.target = self
        menu.addItem(statusItem)

        if engine.status == .needsPermission {
            let grantItem = NSMenuItem(title: tr("🔑 前往系统设置开启「输入监控」...", "🔑 Open Input Monitoring Settings...", "🔑 Abrir ajustes de Monitoramento de Entrada..."), action: #selector(openInputMonitoringSettings), keyEquivalent: "")
            grantItem.target = self
            menu.addItem(grantItem)
        }

        // Live stats
        let statsTitle = tr(
            "⌨️ 今日击键 \(engine.totalKeystrokesToday)  ·  KPM \(engine.currentKPM)  ·  摸猫 \(physics.totalPetSessions) 次",
            "⌨️ Today \(engine.totalKeystrokesToday)  ·  KPM \(engine.currentKPM)  ·  Pets \(physics.totalPetSessions)",
            "⌨️ Hoje \(engine.totalKeystrokesToday)  ·  KPM \(engine.currentKPM)  ·  Carinhos \(physics.totalPetSessions)")
        let statsItem = NSMenuItem(title: statsTitle, action: nil, keyEquivalent: "")
        statsItem.isEnabled = false
        menu.addItem(statsItem)

        menu.addItem(.separator())

        // Essential everyday actions only — everything else lives cleanly in the unified Settings window!
        let visItem = NSMenuItem(title: isPetVisible ? tr("隐藏桌宠", "Hide Pet", "Ocultar gatinho") : tr("显示桌宠", "Show Pet", "Mostrar gatinho"), action: #selector(toggleVisibility), keyEquivalent: "")
        visItem.target = self
        menu.addItem(visItem)

        // Quick Cat Skin switcher submenu
        let coatMenu = NSMenu()
        let library = SkinLibrary.shared
        let customActive = library.skin(id: settings.customSkin) != nil
        for coat in CatCoatTheme.allCases {
            let coatTitle: String
            coatTitle = coat.displayName
            let item = NSMenuItem(title: coatTitle, action: #selector(selectCoatTheme(_:)), keyEquivalent: "")
            item.target = self
            item.representedObject = coat.rawValue
            item.state = !customActive && settings.coatTheme == coat ? .on : .off
            coatMenu.addItem(item)
        }
        if !library.skins.isEmpty {
            coatMenu.addItem(.separator())
            for skin in library.skins {
                let item = NSMenuItem(title: library.displayName(for: skin), action: #selector(selectCustomSkin(_:)), keyEquivalent: "")
                item.target = self
                item.representedObject = skin.id
                item.state = settings.customSkin == skin.id ? .on : .off
                if !skin.author.isEmpty { item.toolTip = tr("作者：\(skin.author)", "Author: \(skin.author)", "Autor: \(skin.author)") }
                coatMenu.addItem(item)
            }
        }
        coatMenu.addItem(.separator())
        let importItem = NSMenuItem(title: tr("导入皮肤…", "Import Skin…", "Importar skin…"), action: #selector(showDressingRoom), keyEquivalent: "")
        importItem.target = self
        coatMenu.addItem(importItem)
        let workshopItem = NSMenuItem(title: tr("打开皮肤工坊…", "Open Skin Workshop…", "Abrir oficina de skins…"), action: #selector(openSkinWorkshop), keyEquivalent: "")
        workshopItem.target = self
        coatMenu.addItem(workshopItem)
        let openDirItem = NSMenuItem(title: tr("打开皮肤文件夹", "Open Skins Folder", "Abrir pasta de skins"), action: #selector(openSkinsFolder), keyEquivalent: "")
        openDirItem.target = self
        coatMenu.addItem(openDirItem)
        if !library.skins.isEmpty {
            coatMenu.addItem(.separator())
            let renameMenu = NSMenu()
            for skin in library.skins {
                let item = NSMenuItem(title: "✏️ " + library.displayName(for: skin), action: #selector(renameCustomSkin(_:)), keyEquivalent: "")
                item.target = self
                item.representedObject = skin.id
                renameMenu.addItem(item)
            }
            let renameParent = NSMenuItem(title: tr("✏️ 重命名皮肤…", "✏️ Rename Skin…", "✏️ Renomear skin…"), action: nil, keyEquivalent: "")
            renameParent.submenu = renameMenu
            coatMenu.addItem(renameParent)

            let deleteMenu = NSMenu()
            for skin in library.skins {
                let delTitle = "🗑 " + library.displayName(for: skin)
                let item = NSMenuItem(title: delTitle, action: #selector(deleteCustomSkin(_:)), keyEquivalent: "")
                item.attributedTitle = NSAttributedString(
                    string: delTitle,
                    attributes: [.foregroundColor: NSColor.systemRed, .font: NSFont.menuFont(ofSize: 0)]
                )
                item.target = self
                item.representedObject = skin.id
                deleteMenu.addItem(item)
            }
            let deleteTitle = tr("🗑 删除已导入皮肤…", "🗑 Delete Imported Skin…", "🗑 Excluir skin importada…")
            let deleteParent = NSMenuItem(title: deleteTitle, action: nil, keyEquivalent: "")
            deleteParent.attributedTitle = NSAttributedString(
                string: deleteTitle,
                attributes: [.foregroundColor: NSColor.systemRed, .font: NSFont.boldSystemFont(ofSize: NSFont.systemFontSize)]
            )
            deleteParent.submenu = deleteMenu
            coatMenu.addItem(deleteParent)

            let batchTitle = tr("📦 批量删除皮肤…", "📦 Batch Delete Skins…", "📦 Excluir várias skins…")
            let batchItem = NSMenuItem(title: batchTitle, action: #selector(openSkinManagerWindow), keyEquivalent: "")
            batchItem.attributedTitle = NSAttributedString(
                string: batchTitle,
                attributes: [.foregroundColor: NSColor.systemRed, .font: NSFont.menuFont(ofSize: 0)]
            )
            batchItem.target = self
            coatMenu.addItem(batchItem)
        }
        let coatParent = NSMenuItem(title: tr("猫咪皮肤 🐱", "Cat Skin 🐱", "Skin do gato 🐱"), action: nil, keyEquivalent: "")
        coatParent.submenu = coatMenu
        menu.addItem(coatParent)

        let websiteItem = NSMenuItem(
            title: tr("🌐 软件官网 (kurocat.cc)", "🌐 Website (kurocat.cc)", "🌐 Site oficial (kurocat.cc)"),
            action: #selector(openWebsite),
            keyEquivalent: ""
        )
        websiteItem.target = self
        menu.addItem(websiteItem)

        let levelKbItem = NSMenuItem(
            title: tr("键盘水平摆放（平行屏幕）", "Level Keyboard (Parallel to Screen)", "Teclado nivelado (paralelo à tela)"),
            action: #selector(toggleLevelKeyboard),
            keyEquivalent: ""
        )
        levelKbItem.target = self
        levelKbItem.state = settings.levelKeyboard ? .on : .off
        menu.addItem(levelKbItem)

        let settingsItem = NSMenuItem(title: tr("⚙️ 设置…", "⚙️ Settings…", "⚙️ Configurações…"), action: #selector(openSettingsWindow), keyEquivalent: ",")
        settingsItem.target = self
        menu.addItem(settingsItem)

        menu.addItem(.separator())

        if let ver = availableUpdateVersion {
            let title = isUpdating
                ? tr("⏳ 正在下载更新 \(ver)...", "⏳ Downloading update \(ver)...", "⏳ Baixando atualização \(ver)...")
                : tr("🆕 发现新版本 \(ver) — 点击自动更新", "🆕 Update Available \(ver) — Click to Update", "🆕 Nova versão \(ver) — clique para atualizar")
            let updateItem = NSMenuItem(title: title, action: #selector(openUpdateDownload), keyEquivalent: "")
            updateItem.target = self
            updateItem.isEnabled = !isUpdating
            menu.addItem(updateItem)
        } else {
            let checkItem = NSMenuItem(
                title: tr("检查更新 (v\(currentAppVersion))", "Check for Updates (v\(currentAppVersion))", "Verificar atualizações (v\(currentAppVersion))"),
                action: #selector(manualCheckForUpdates),
                keyEquivalent: ""
            )
            checkItem.target = self
            menu.addItem(checkItem)
        }

        menu.addItem(.separator())

        let relaunchItem = NSMenuItem(title: tr("重启 KuroCat", "Restart KuroCat", "Reiniciar KuroCat"), action: #selector(relaunchApp), keyEquivalent: "")
        relaunchItem.target = self
        menu.addItem(relaunchItem)

        let quitItem = NSMenuItem(title: tr("退出 KuroCat", "Quit KuroCat", "Sair do KuroCat"), action: #selector(quitApp), keyEquivalent: "q")
        quitItem.target = self
        menu.addItem(quitItem)

        return menu
    }

    @objc private func openSettingsWindow() {
        if settingsWindow == nil {
            let ctrl = SettingsWindowController()
            ctrl.onSettingsChanged = { [weak self] in
                self?.wakeToActive(immediateRedraw: true)
                self?.dressingRoom?.refresh()
            }
            ctrl.onScaleChanged = { [weak self] newScale in
                self?.overlayWindow.applyScale(newScale)
                self?.wakeToActive()
            }
            ctrl.onResetPoseAndPosition = { [weak self] in
                self?.overlayWindow.applyScale(1.0)
                self?.overlayWindow.resetPosition()
                self?.wakeToActive(immediateRedraw: true)
            }
            ctrl.onOpenDressingRoom = { [weak self] in
                self?.showDressingRoom()
            }
            ctrl.onOpenSkinManager = { [weak self] in
                self?.openSkinManagerWindow()
            }
            ctrl.onSkinsDirectoryChanged = { [weak self] in
                self?.overlayWindow.catView.needsDisplay = true
                self?.dressingRoom?.refresh()
                self?.skinManagerWindow?.refresh()
            }
            ctrl.onRenameSkin = { [weak self] id in
                self?.performRenameSkin(id: id)
            }
            ctrl.onDeleteSkin = { [weak self] id in
                self?.performDeleteSkin(id: id)
            }
            ctrl.onImportKeyboardSkin = { [weak self] in
                self?.promptImportKeyboardSkin()
            }
            ctrl.onApplyWindowLevel = { [weak self] in
                self?.overlayWindow.applyWindowLevelAndBehavior()
                self?.dressingRoom?.syncWindowLevel()
                self?.settingsWindow?.syncWindowLevel()
                self?.skinManagerWindow?.syncWindowLevel()
                self?.updatePromptController?.syncWindowLevel()
                self?.syncWindowTopmost()
            }
            ctrl.onApplyOBSMode = { [weak self] in
                self?.overlayWindow.applyOBSMode()
                self?.wakeToActive(immediateRedraw: true)
            }
            ctrl.onAutoHideChanged = { [weak self] in
                guard let self else { return }
                if !CatSettings.shared.autoHideFullscreen && self.hiddenByFullscreen {
                    self.hiddenByFullscreen = false
                    self.overlayWindow.alphaValue = 1.0
                    self.wakeToActive(immediateRedraw: true)
                }
            }
            ctrl.getAutoStart = { [weak self] in
                self?.isAutoStartEnabled() ?? false
            }
            ctrl.setAutoStart = { [weak self] on in
                _ = self?.setAutoStartEnabled(on)
            }
            ctrl.onLanguageChanged = { [weak self] in
                self?.wakeToActive(immediateRedraw: true)
            }
            releaseOnClose(ctrl.window) { [weak self] in self?.settingsWindow = nil }
            settingsWindow = ctrl
        }
        settingsWindow?.present()
    }

    private func releaseOnClose(_ window: NSWindow?, clear: @escaping () -> Void) {
        guard let window else { return }
        var token: NSObjectProtocol?
        token = NotificationCenter.default.addObserver(
            forName: NSWindow.willCloseNotification,
            object: window,
            queue: .main
        ) { _ in
            if let token {
                NotificationCenter.default.removeObserver(token)
            }
            clear()
        }
    }

    private func purgeSkinCachesIfNeeded() {
        if CatSettings.shared.activeSkin.renderStyle != .beagle {
            BeagleAtlas.purgeCache()
        }
        CatRenderer.purgeVolatileCaches()
    }

    // MARK: - Actions

    @objc private func openInputMonitoringSettings() {
        if InputEngine.shared.status == .needsRestart {
            NSApp.activate(ignoringOtherApps: true)
            let alert = NSAlert()
            alert.messageText = loc("已授权，键盘监听启动失败", "Authorized, but key capture failed to start", "Autorizado, mas a captura de teclas não iniciou")
            alert.informativeText = loc("请重启 KuroCat，让系统重新建立键盘监听。\n当前程序位置：\(Bundle.main.bundlePath)",
                                        "Restart KuroCat so macOS can set up key capture again.\nApp location: \(Bundle.main.bundlePath)",
                                        "Reinicie o KuroCat para o macOS refazer a captura de teclas.\nLocal do app: \(Bundle.main.bundlePath)")
            alert.addButton(withTitle: loc("重启 KuroCat", "Restart KuroCat", "Reiniciar KuroCat"))
            alert.addButton(withTitle: loc("打开输入监控", "Open Input Monitoring", "Abrir Monitoramento de Entrada"))
            alert.addButton(withTitle: loc("取消", "Cancel", "Cancelar"))
            let response = runModalAlert(alert)
            if response == .alertFirstButtonReturn {
                relaunchApp()
                return
            }
            if response != .alertSecondButtonReturn { return }
        }
        InputEngine.shared.requestPermissionsAndOpenSettings()
    }

    @objc private func toggleVisibility() {
        isPetVisible.toggle()
        if isPetVisible {
            lastFrameTime = CACurrentMediaTime()
            overlayWindow.orderFrontRegardless()
            wakeToActive(immediateRedraw: true)
        } else {
            configureRenderTimer(for: .paused)
            overlayWindow.orderOut(nil)
        }
    }

    @objc private func selectTabletMode(_ sender: NSMenuItem) {
        if let mode = sender.representedObject as? Int {
            CatSettings.shared.tabletMode = mode
            wakeToActive(immediateRedraw: true)
        }
    }

    @objc private func toggleClickThrough() {
        CatSettings.shared.fullClickThrough.toggle()
    }

    @objc private func toggleFullScreenOverlay() {
        CatSettings.shared.fullScreenOverlay.toggle()
        overlayWindow.applyWindowLevelAndBehavior()
        dressingRoom?.syncWindowLevel()
        settingsWindow?.syncWindowLevel()
        updatePromptController?.syncWindowLevel()
        syncWindowTopmost()
    }

    // MARK: - Topmost Watchdog (Space / App Activation)

    private func startWindowWatchdog() {
        let nc = NSWorkspace.shared.notificationCenter
        nc.addObserver(self, selector: #selector(handleWorkspaceStateChanged),
                       name: NSWorkspace.activeSpaceDidChangeNotification, object: nil)
        nc.addObserver(self, selector: #selector(handleWorkspaceStateChanged),
                       name: NSWorkspace.didActivateApplicationNotification, object: nil)

        windowWatchdogTimer?.invalidate()
        let timer = Timer(timeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.syncWindowTopmost()
        }
        timer.tolerance = 0.4
        RunLoop.main.add(timer, forMode: .common)
        windowWatchdogTimer = timer
    }

    @objc private func handleWorkspaceStateChanged() {
        syncWindowTopmost()
    }

    private func syncWindowTopmost() {
        guard let overlayWindow, isPetVisible, !hiddenByFullscreen, !screensAsleep, !isPresentingModalAlert else { return }
        let s = CatSettings.shared
        let obsBg = (s.obsCaptureMode > 0 && s.obsHideDesktop)
        if obsBg { return }
        if s.fullScreenOverlay {
            overlayWindow.orderFrontRegardless()
        }
    }

    /// Presents a modal NSAlert strictly above the overlay window (whose .popUpMenuWindow level 101
    /// would otherwise obscure and intercept clicks on a standard .modalPanel level-8 alert).
    @discardableResult
    private func runModalAlert(_ alert: NSAlert) -> NSApplication.ModalResponse {
        runWithOverlayYielding {
            alert.window.level = NSWindow.Level(rawValue: max(
                Int(CGWindowLevelForKey(.screenSaverWindow)),
                Int(CGWindowLevelForKey(.popUpMenuWindow)) + 2
            ))
            // Note: .canJoinAllSpaces and .moveToActiveSpace are mutually exclusive in AppKit
            // and combining them throws NSInternalInconsistencyException (bypassing Swift defer).
            alert.window.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]
            NSApp.activate(ignoringOtherApps: true)
            alert.window.orderFrontRegardless()
            return alert.runModal()
        }
    }

    private func runWithOverlayYielding<T>(_ body: () -> T) -> T {
        isPresentingModalAlert = true
        overlayWindow?.catView.forceClickThroughForModal = true
        let obsBg = (CatSettings.shared.obsCaptureMode > 0 && CatSettings.shared.obsHideDesktop)
        if let overlayWindow, !obsBg {
            overlayWindow.level = .floating
            overlayWindow.ignoresMouseEvents = true
        }
        defer {
            isPresentingModalAlert = false
            overlayWindow?.catView.forceClickThroughForModal = false
            if let overlayWindow, !obsBg {
                overlayWindow.applyWindowLevelAndBehavior()
                syncWindowTopmost()
            }
        }
        return body()
    }

    // MARK: - Launch at Login (macOS LaunchAgent — works reliably for signed & ad-hoc builds)

    private var launchAgentURL: URL {
        let dir = FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent("Library/LaunchAgents", isDirectory: true)
        return dir.appendingPathComponent("com.chanke.kurocat.plist")
    }

    private func isAutoStartEnabled() -> Bool {
        FileManager.default.fileExists(atPath: launchAgentURL.path)
    }

    @discardableResult
    private func setAutoStartEnabled(_ enable: Bool) -> Bool {
        let fm = FileManager.default
        let url = launchAgentURL
        if enable {
            do {
                try fm.createDirectory(at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
                let appPath = Bundle.main.bundlePath
                let plist: [String: Any] = [
                    "Label": "com.chanke.kurocat",
                    "ProgramArguments": ["/usr/bin/open", "-a", appPath],
                    "RunAtLoad": true,
                    "KeepAlive": false
                ]
                let data = try PropertyListSerialization.data(fromPropertyList: plist, format: .xml, options: 0)
                try data.write(to: url, options: .atomic)
                return true
            } catch {
                return false
            }
        } else {
            do {
                if fm.fileExists(atPath: url.path) {
                    try fm.removeItem(at: url)
                }
                return true
            } catch {
                return false
            }
        }
    }

    @objc private func toggleAutoStart() {
        _ = setAutoStartEnabled(!isAutoStartEnabled())
    }

    @objc private func toggleKeyBadges() {
        CatSettings.shared.showKeyBadges.toggle()
        wakeToActive(immediateRedraw: true)
    }

    @objc private func toggleMouseBadges() {
        CatSettings.shared.showMouseBadges.toggle()
        wakeToActive(immediateRedraw: true)
    }

    @objc private func toggleKeyLegends() {
        CatSettings.shared.showKeyLegends.toggle()
        wakeToActive(immediateRedraw: true)
    }

    @objc private func toggleFootEgg() {
        CatSettings.shared.footEasterEgg.toggle()
        wakeToActive(immediateRedraw: true)
    }

    @objc private func toggleTailEgg() {
        CatSettings.shared.tailEasterEgg.toggle()
        wakeToActive(immediateRedraw: true)
    }

    @objc private func toggleKeyboardMarquee() {
        CatSettings.shared.keyboardMarquee.toggle()
        wakeToActive(immediateRedraw: true)
    }

    @objc private func toggleLevelKeyboard() {
        CatSettings.shared.levelKeyboard.toggle()
        wakeToActive(immediateRedraw: true)
        settingsWindow?.refresh()
    }

    @objc private func togglePetInteraction() {
        CatSettings.shared.petInteraction.toggle()
    }

    @objc private func selectOBSCaptureMode(_ sender: NSMenuItem) {
        guard let num = sender.representedObject as? NSNumber else { return }
        CatSettings.shared.obsCaptureMode = num.intValue
        if num.intValue == 0 {
            CatSettings.shared.obsHideDesktop = false
        }
        overlayWindow.applyOBSMode()
        wakeToActive(immediateRedraw: true)
    }

    @objc private func toggleOBSHideDesktop() {
        CatSettings.shared.obsHideDesktop.toggle()
        overlayWindow.applyOBSMode()
        wakeToActive(immediateRedraw: true)
    }

    @objc private func toggleAutoHideFullscreen() {
        CatSettings.shared.autoHideFullscreen.toggle()
        if !CatSettings.shared.autoHideFullscreen && hiddenByFullscreen {
            hiddenByFullscreen = false
            overlayWindow.alphaValue = 1.0
            wakeToActive(immediateRedraw: true)
        }
    }

    private static func isOtherAppFullscreen() -> Bool {
        guard let front = NSWorkspace.shared.frontmostApplication,
              front.processIdentifier != ProcessInfo.processInfo.processIdentifier,
              front.bundleIdentifier != "com.apple.finder" else {
            return false
        }
        guard let list = CGWindowListCopyWindowInfo([.optionOnScreenOnly, .excludeDesktopElements], kCGNullWindowID) as? [[String: Any]] else {
            return false
        }
        let pid = front.processIdentifier
        for w in list {
            guard let ownerPID = w[kCGWindowOwnerPID as String] as? pid_t, ownerPID == pid,
                  let layer = w[kCGWindowLayer as String] as? Int, layer == 0,
                  let boundsDict = w[kCGWindowBounds as String] as? [String: Any],
                  let rect = CGRect(dictionaryRepresentation: boundsDict as CFDictionary) else { continue }
            for screen in NSScreen.screens {
                if rect.width >= screen.frame.width - 2 && rect.height >= screen.frame.height - 2 {
                    return true
                }
            }
        }
        return false
    }

    @objc private func selectLanguage(_ sender: NSMenuItem) {
        guard let code = sender.representedObject as? String else { return }
        CatSettings.shared.language = code
        wakeToActive(immediateRedraw: true)
    }

    @objc private func selectScale(_ sender: NSMenuItem) {
        guard let num = sender.representedObject as? NSNumber else { return }
        overlayWindow.applyScale(CGFloat(num.doubleValue))
        wakeToActive()
    }

    @objc private func selectCoatTheme(_ sender: NSMenuItem) {
        guard let raw = sender.representedObject as? String,
              let coat = CatCoatTheme(rawValue: raw) else { return }
        CatSettings.shared.overrideSkinIris = false
        CatSettings.shared.customSkin = ""
        CatSettings.shared.coatTheme = coat
        // Auto-pair signature eye color (still freely overridable via 瞳色切换!)
        switch coat {
        case .classicBlack:
            CatSettings.shared.eyeTheme = .neonChartreuse
        case .siamese:
            CatSettings.shared.eyeTheme = .siameseBlue
        case .shorthair:
            CatSettings.shared.eyeTheme = .referenceHazel
        case .orangeWhite:
            CatSettings.shared.eyeTheme = .warmAmber
        }
        purgeSkinCachesIfNeeded()
        wakeToActive(immediateRedraw: true)
        dressingRoom?.refresh()
        settingsWindow?.refresh()
    }

    // MARK: - Imported skins (换装间)

    private static let skinWorkshopURLString = "https://chanke17.github.io/KuroCat-Workshop/"
    private static let keyboardPlazaURLString = "https://chanke17.github.io/KuroCat-Workshop/#keyboards"
    private static let websiteURLString = "https://kurocat.cc"

    func application(_ application: NSApplication, open urls: [URL]) {
        for url in urls {
            importDroppedFile(from: url)
        }
    }

    private func importDroppedFile(from url: URL) {
        if KeyboardSkinSpec.looksLikeKeyboardJSON(url) {
            importKeyboardSkin(from: url)
        } else if url.pathExtension.lowercased() == "kurocat" {
            importSkin(from: url)
        }
    }

    @objc private func openSkinWorkshop() {
        if let url = URL(string: Self.skinWorkshopURLString) {
            NSWorkspace.shared.open(url)
        }
    }

    @objc private func openWebsite() {
        if let url = URL(string: Self.websiteURLString) {
            NSWorkspace.shared.open(url)
        }
    }

    @objc private func openSkinsFolder() {
        let dir = SkinLibrary.directory
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        NSWorkspace.shared.open(dir)
    }

    @objc private func showDressingRoom() {
        if dressingRoom == nil {
            let controller = DressingRoomWindowController()
            controller.onImport = { [weak self] url in self?.importSkin(from: url) }
            releaseOnClose(controller.window) { [weak self] in self?.dressingRoom = nil }
            dressingRoom = controller
        }
        dressingRoom?.present()
    }

    /// Validates and stores a `.kurocat`, then wears it. Invalid files get a Chinese explanation.
    private func importSkin(from url: URL) {
        do {
            let id = try SkinLibrary.shared.importFile(url)
            CatSettings.shared.overrideSkinIris = false
            CatSettings.shared.customSkin = id
            let skin = SkinLibrary.shared.skin(id: id)
            let skinName = skin?.name ?? loc("新皮肤", "new skin", "nova skin")
            var message = loc("已换上「\(skinName)」", "Now wearing \"\(skinName)\"", "Vestindo \"\(skinName)\"")
            if let author = skin?.author, !author.isEmpty { message += loc("（作者：\(author)）", " (by \(author))", " (por \(author))") }
            if let sk = skin, SkinLibrary.shared.skins.filter({ $0.name == sk.name }).count > 1 {
                message += loc(" · 存在同名皮肤，已附加短编号区分", " · A skin with this name exists; a short ID was added", " · Já existe uma skin com esse nome; foi adicionado um código curto")
            }
            dressingRoom?.setStatus("✓ " + message, kind: 1)
            wakeToActive(immediateRedraw: true)
            dressingRoom?.refresh()
            settingsWindow?.refresh()
            skinManagerWindow?.refresh()
        } catch {
            let reason = error.localizedDescription
            // With 换装间 open the red status line explains it in place; an alert is only for drops onto the cat.
            if let room = dressingRoom, room.isVisible {
                room.setStatus(loc("没能导入「\(url.lastPathComponent)」：\(reason)",
                                   "Couldn't import \"\(url.lastPathComponent)\": \(reason)",
                                   "Não foi possível importar \"\(url.lastPathComponent)\": \(reason)"), kind: 2)
                return
            }
            let alert = NSAlert()
            alert.alertStyle = .warning
            alert.messageText = loc("无法导入这个皮肤", "Failed to Import Skin", "Não foi possível importar a skin")
            alert.informativeText = "「\(url.lastPathComponent)」：\(reason)"
            alert.addButton(withTitle: loc("知道了", "OK", "OK"))
            runModalAlert(alert)
        }
    }

    @objc private func selectCustomSkin(_ sender: NSMenuItem) {
        guard let id = sender.representedObject as? String else { return }
        CatSettings.shared.overrideSkinIris = false
        CatSettings.shared.customSkin = id
        purgeSkinCachesIfNeeded()
        wakeToActive(immediateRedraw: true)
        dressingRoom?.refresh()
        settingsWindow?.refresh()
    }

    @objc private func renameCustomSkin(_ sender: NSMenuItem) {
        guard let id = sender.representedObject as? String else { return }
        performRenameSkin(id: id)
    }

    private func performRenameSkin(id: String) {
        guard let skin = SkinLibrary.shared.skin(id: id) else { return }
        let alert = NSAlert()
        alert.messageText = loc("重命名皮肤", "Rename Skin", "Renomear skin")
        alert.informativeText = loc("为「\(skin.name)」输入新的名称（最多 40 字）：",
                                    "Enter a new name for \"\(skin.name)\" (up to 40 characters):",
                                    "Digite um novo nome para \"\(skin.name)\" (até 40 caracteres):")
        alert.addButton(withTitle: loc("保存", "Rename", "Renomear"))
        alert.addButton(withTitle: loc("取消", "Cancel", "Cancelar"))

        let input = NSTextField(frame: NSRect(x: 0, y: 0, width: 260, height: 24))
        input.stringValue = skin.name
        alert.accessoryView = input
        alert.window.initialFirstResponder = input
        guard runModalAlert(alert) == .alertFirstButtonReturn else { return }

        let newName = input.stringValue.trimmingCharacters(in: .whitespacesAndNewlines)
        if newName.isEmpty || newName == skin.name { return }
        do {
            let newId = try SkinLibrary.shared.rename(id: id, newName: newName)
            if CatSettings.shared.customSkin == id {
                CatSettings.shared.customSkin = newId
            }
            dressingRoom?.refresh()
            settingsWindow?.refresh()
            skinManagerWindow?.refresh()
        } catch {
            let failed = NSAlert()
            failed.alertStyle = .warning
            failed.messageText = loc("重命名失败", "Rename Failed", "Falha ao renomear")
            failed.informativeText = error.localizedDescription
            runModalAlert(failed)
        }
    }

    @objc private func deleteCustomSkin(_ sender: NSMenuItem) {
        guard let id = sender.representedObject as? String else { return }
        performDeleteSkin(id: id)
    }

    private func performDeleteSkin(id: String) {
        guard let skin = SkinLibrary.shared.skin(id: id) else { return }
        let alert = NSAlert()
        alert.alertStyle = .warning
        alert.messageText = loc("删除皮肤「\(skin.name)」？", "Delete skin \"\(skin.name)\"?", "Excluir a skin \"\(skin.name)\"?")
        alert.informativeText = loc("会从皮肤文件夹里删除这个文件，删除后需要重新导入。",
                                    "The file will be removed from the skins folder; you'll need to import it again to use it.",
                                    "O arquivo será apagado da pasta de skins; para usá-la de novo será preciso importar outra vez.")
        let delBtn = alert.addButton(withTitle: loc("🗑 删除", "🗑 Delete", "🗑 Excluir"))
        if #available(macOS 11.0, *) { delBtn.hasDestructiveAction = true }
        alert.addButton(withTitle: loc("取消", "Cancel", "Cancelar"))
        guard runModalAlert(alert) == .alertFirstButtonReturn else { return }
        if !SkinLibrary.shared.remove(id: id) {
            let failed = NSAlert()
            failed.messageText = loc("删除失败", "Delete Failed", "Falha ao excluir")
            failed.informativeText = loc("无法删除 ", "Couldn't delete ", "Não foi possível excluir ") + "\(SkinLibrary.directory.appendingPathComponent("\(id).kurocat").path)"
            runModalAlert(failed)
            return
        }
        if CatSettings.shared.customSkin == id { CatSettings.shared.customSkin = "" }
        wakeToActive(immediateRedraw: true)
        dressingRoom?.refresh()
        settingsWindow?.refresh()
        skinManagerWindow?.refresh()
    }

    @objc private func openSkinManagerWindow() {
        if skinManagerWindow == nil {
            let ctrl = SkinBatchDeleteWindowController()
            ctrl.onBatchDelete = { [weak self] ids in
                self?.performBatchDeleteSkins(ids: ids) ?? false
            }
            releaseOnClose(ctrl.window) { [weak self] in self?.skinManagerWindow = nil }
            skinManagerWindow = ctrl
        }
        skinManagerWindow?.present()
    }

    @discardableResult
    private func performBatchDeleteSkins(ids: [String]) -> Bool {
        let targets = ids.compactMap { SkinLibrary.shared.skin(id: $0) }
        guard !targets.isEmpty else { return false }
        let previewNames = targets.prefix(6).map { "• " + SkinLibrary.shared.displayName(for: $0) }.joined(separator: "\n")
        let extra = targets.count > 6
            ? loc("\n……等共 \(targets.count) 款皮肤", "\n...and \(targets.count - 6) more", "\n...e mais \(targets.count - 6)")
            : ""

        let alert = NSAlert()
        alert.alertStyle = .warning
        alert.messageText = loc("确认批量删除这 \(targets.count) 款皮肤？",
                                "Delete \(targets.count) selected skin(s)?",
                                "Excluir \(targets.count) skin(s) selecionada(s)?")
        alert.informativeText = loc("以下 .kurocat 文件会从皮肤文件夹中删除，删除后需重新导入：\n\n",
                                    "These .kurocat files will be permanently removed from your skins folder:\n\n",
                                    "Estes arquivos .kurocat serão apagados da pasta de skins; para usá-los de novo será preciso importar outra vez:\n\n") + previewNames + extra
        let delBtn = alert.addButton(withTitle: loc("🗑 全部删除 (\(targets.count))", "🗑 Delete \(targets.count)", "🗑 Excluir \(targets.count)"))
        if #available(macOS 11.0, *) { delBtn.hasDestructiveAction = true }
        alert.addButton(withTitle: loc("取消", "Cancel", "Cancelar"))
        guard runModalAlert(alert) == .alertFirstButtonReturn else { return false }

        let targetIDs = targets.map { $0.id }
        let result = SkinLibrary.shared.removeMany(ids: targetIDs)
        if result.removed.contains(CatSettings.shared.customSkin) {
            CatSettings.shared.customSkin = ""
        }
        if !result.failed.isEmpty {
            let failedAlert = NSAlert()
            failedAlert.alertStyle = .warning
            failedAlert.messageText = loc("部分皮肤未能删除", "Some skins could not be deleted", "Algumas skins não puderam ser excluídas")
            failedAlert.informativeText = loc("已删除 \(result.removed.count) 款，有 \(result.failed.count) 款文件无法删除。",
                                              "Deleted \(result.removed.count), failed \(result.failed.count).",
                                              "\(result.removed.count) excluída(s); \(result.failed.count) não puderam ser apagada(s).")
            runModalAlert(failedAlert)
        }
        wakeToActive(immediateRedraw: true)
        dressingRoom?.refresh()
        settingsWindow?.refresh()
        skinManagerWindow?.refresh()
        return !result.removed.isEmpty
    }

    @objc private func selectKeyboardTheme(_ sender: NSMenuItem) {
        guard let raw = sender.representedObject as? String,
              let theme = KeyboardTheme(rawValue: raw) else { return }
        CatSettings.shared.customKeyboard = ""
        CatSettings.shared.keyboardTheme = theme
        wakeToActive(immediateRedraw: true)
    }

    @objc private func selectCustomKeyboard(_ sender: NSMenuItem) {
        guard let id = sender.representedObject as? String else { return }
        CatSettings.shared.customKeyboard = id
        wakeToActive(immediateRedraw: true)
    }

    @objc private func openKeyboardPlaza() {
        if let url = URL(string: Self.keyboardPlazaURLString) {
            NSWorkspace.shared.open(url)
        }
    }

    @objc private func openKeyboardsFolder() {
        let dir = KeyboardSkinLibrary.directory
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        NSWorkspace.shared.open(dir)
    }

    @objc private func promptImportKeyboardSkin() {
        let panel = NSOpenPanel()
        panel.title = loc("选择键盘皮肤文件 (.kurokb / .json)", "Choose Keyboard Skin (.kurokb / .json)", "Escolher skin de teclado (.kurokb / .json)")
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false
        panel.allowedFileTypes = ["kurokb", "json"]
        let resp = runWithOverlayYielding {
            panel.level = NSWindow.Level(rawValue: max(
                Int(CGWindowLevelForKey(.screenSaverWindow)),
                Int(CGWindowLevelForKey(.popUpMenuWindow)) + 2
            ))
            NSApp.activate(ignoringOtherApps: true)
            return panel.runModal()
        }
        if resp == .OK, let url = panel.url {
            importKeyboardSkin(from: url)
        }
    }

    private func importKeyboardSkin(from url: URL) {
        do {
            let id = try KeyboardSkinLibrary.shared.importFile(url)
            CatSettings.shared.customKeyboard = id
            let kb = KeyboardSkinLibrary.shared.skin(id: id)
            let kbName = kb?.name ?? loc("新键盘", "new keyboard", "novo teclado")
            var message = loc("已换上键盘皮肤「\(kbName)」", "Keyboard skin \"\(kbName)\" applied", "Skin de teclado \"\(kbName)\" aplicada")
            if let author = kb?.author, !author.isEmpty { message += loc("（作者：\(author)）", " (by \(author))", " (por \(author))") }
            dressingRoom?.setStatus("✓ " + message, kind: 1)
            wakeToActive(immediateRedraw: true)
            settingsWindow?.refresh()
        } catch {
            let alert = NSAlert()
            alert.alertStyle = .warning
            alert.messageText = loc("无法导入这个键盘皮肤", "Failed to Import Keyboard Skin", "Não foi possível importar a skin de teclado")
            alert.informativeText = "「\(url.lastPathComponent)」：\(error.localizedDescription)"
            alert.addButton(withTitle: loc("知道了", "OK", "OK"))
            runModalAlert(alert)
        }
    }

    @objc private func deleteCustomKeyboard(_ sender: NSMenuItem) {
        guard let id = sender.representedObject as? String, let kb = KeyboardSkinLibrary.shared.skin(id: id) else { return }
        let alert = NSAlert()
        alert.messageText = loc("删除键盘皮肤「\(kb.name)」？", "Delete keyboard skin \"\(kb.name)\"?", "Excluir a skin de teclado \"\(kb.name)\"?")
        alert.informativeText = loc("会从键盘皮肤文件夹里删除这个文件，删除后需要重新导入。",
                                    "The file will be removed from the keyboard skins folder; you'll need to import it again to use it.",
                                    "O arquivo será apagado da pasta de skins de teclado; para usá-la de novo será preciso importar outra vez.")
        alert.addButton(withTitle: loc("删除", "Delete", "Excluir"))
        alert.addButton(withTitle: loc("取消", "Cancel", "Cancelar"))
        guard runModalAlert(alert) == .alertFirstButtonReturn else { return }
        if !KeyboardSkinLibrary.shared.remove(id: id) {
            let failed = NSAlert()
            failed.messageText = loc("删除失败", "Delete Failed", "Falha ao excluir")
            failed.informativeText = loc("无法删除 ", "Couldn't delete ", "Não foi possível excluir ") + "\(KeyboardSkinLibrary.directory.appendingPathComponent("\(id).kurokb").path)"
            runModalAlert(failed)
            return
        }
        if CatSettings.shared.customKeyboard == id { CatSettings.shared.customKeyboard = "" }
        wakeToActive(immediateRedraw: true)
        settingsWindow?.refresh()
    }

    @objc private func selectEyeTheme(_ sender: NSMenuItem) {
        guard let raw = sender.representedObject as? String,
              let theme = EyeColorTheme(rawValue: raw) else { return }
        CatSettings.shared.overrideSkinIris = true
        CatSettings.shared.eyeTheme = theme
        dressingRoom?.refresh()
        settingsWindow?.refresh()
        wakeToActive(immediateRedraw: true)
    }

    @objc private func resetPosition() {
        overlayWindow.resetPosition()
    }

    @objc private func runDemoBurst() {
        let keys: [PhysicalKey] = [.w, .a, .s, .d, .f, .j, .o, .e, .space, .leftShift]
        wakeToActive()
        for (idx, k) in keys.enumerated() {
            DispatchQueue.main.asyncAfter(deadline: .now() + Double(idx) * 0.13) { [weak self] in
                guard let self else { return }
                let now = CACurrentMediaTime()
                self.lastDemoKeyTime = now
                self.overlayWindow.catView.physics.triggerKeyDown(k, at: now)
                self.wakeToActive()
            }
        }
    }

    @objc private func testHourlyToast() {
        let cal = Calendar.current
        let now = Date()
        let hour = cal.component(.hour, from: now)
        showHourlyToast(hour: hour)
    }

    @objc private func relaunchApp() {
        let bundleURL = Bundle.main.bundleURL
        let config = NSWorkspace.OpenConfiguration()
        config.createsNewApplicationInstance = true
        NSWorkspace.shared.openApplication(at: bundleURL, configuration: config) { _, _ in
            DispatchQueue.main.async {
                NSApp.terminate(nil)
            }
        }
    }

    @objc private func quitApp() {
        NSApp.terminate(nil)
    }

    // MARK: - Auto Update Checker & In-App Self-Updater

    private static let updatePageURLString = "https://kurocat-download.app.workbuddy.host/?sharecode=JVAABIVZ7L"
    private static let updateVersionJSONURLString = "https://kurocat-download.app.workbuddy.host/version.json?sharecode=JVAABIVZ7L"
    private static let defaultMacAssetURLString = "https://kurocat-download.app.workbuddy.host/downloads/KuroCat-macOS-universal.zip?sharecode=JVAABIVZ7L"

    /// PT-BR fork: the in-app updater downloads unsigned, unhashed binaries from a third-party
    /// mirror, so it is disabled. "Check for Updates" just opens this fork's Releases page.
    private static let forkReleasesURLString = "https://github.com/tobecwb/KuroCat/releases"

    @objc private func manualCheckForUpdates() {
        NSWorkspace.shared.open(URL(string: Self.forkReleasesURLString)!)
    }

    @objc private func openUpdateDownload() {
        performInAppUpdate()
    }

    private func parseUpdateInfo(from data: Data) -> (tag: String, pageURL: URL?, assetURL: URL?)? {
        if let json = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
           let tagName = json["tag_name"] as? String, !tagName.isEmpty {
            let pageStr = (json["html_url"] as? String) ?? Self.updatePageURLString
            let assets = json["assets"] as? [String: Any]
            let macStr = (assets?["macos"] as? String) ?? Self.defaultMacAssetURLString
            return (tagName, URL(string: pageStr), URL(string: macStr))
        }
        if let html = String(data: data, encoding: .utf8) {
            let patterns = [
                #"v(\d+\.\d+\.\d+)\s*已发布"#,
                #"版本\s*v(\d+\.\d+\.\d+)"#
            ]
            for pattern in patterns {
                if let regex = try? NSRegularExpression(pattern: pattern),
                   let match = regex.firstMatch(in: html, range: NSRange(html.startIndex..., in: html)),
                   let r = Range(match.range(at: 1), in: html) {
                    return ("v" + String(html[r]), URL(string: Self.updatePageURLString), URL(string: Self.defaultMacAssetURLString))
                }
            }
        }
        return nil
    }

    private func fetchUpdateEndpoint(_ urlString: String, completion: @escaping ((tag: String, pageURL: URL?, assetURL: URL?)?) -> Void) {
        guard let url = URL(string: urlString) else {
            completion(nil)
            return
        }
        var req = URLRequest(url: url, cachePolicy: .reloadIgnoringLocalCacheData, timeoutInterval: 10)
        req.setValue("KuroCat/\(currentAppVersion)", forHTTPHeaderField: "User-Agent")

        URLSession.shared.dataTask(with: req) { [weak self] data, _, error in
            guard let self else { return }
            guard error == nil, let data, let parsed = self.parseUpdateInfo(from: data) else {
                completion(nil)
                return
            }
            completion(parsed)
        }.resume()
    }

    private func checkForUpdates(manual: Bool) {
        fetchUpdateEndpoint(Self.updateVersionJSONURLString) { [weak self] primary in
            guard let self else { return }
            if let info = primary {
                DispatchQueue.main.async { self.handleUpdateResult(info, manual: manual) }
                return
            }
            self.fetchUpdateEndpoint(Self.updatePageURLString) { [weak self] fallback in
                guard let self else { return }
                DispatchQueue.main.async {
                    if let info = fallback {
                        self.handleUpdateResult(info, manual: manual)
                    } else if manual {
                        self.showInfoAlert(title: loc("检查更新失败", "Update Check Failed", "Falha ao verificar atualizações"),
                                       message: loc("无法连接到更新服务器，请稍后重试或检查网络连接。",
                                                    "Couldn't reach the update server. Try again later or check your connection.",
                                                    "Não foi possível conectar ao servidor de atualizações. Tente mais tarde ou verifique sua conexão."))
                    }
                }
            }
        }
    }

    private func presentNonModalUpdatePrompt(
        title: String,
        message: String,
        primaryTitle: String,
        secondaryTitle: String? = nil,
        cancelTitle: String,
        onPrimary: @escaping () -> Void,
        onSecondary: (() -> Void)? = nil,
        onCancel: @escaping () -> Void
    ) {
        let ctrl = updatePromptController ?? UpdatePromptWindowController()
        if updatePromptController == nil {
            releaseOnClose(ctrl.window) { [weak self] in self?.updatePromptController = nil }
        }
        updatePromptController = ctrl
        ctrl.configure(
            title: title,
            message: message,
            primaryTitle: primaryTitle,
            secondaryTitle: secondaryTitle,
            cancelTitle: cancelTitle,
            onPrimary: onPrimary,
            onSecondary: onSecondary,
            onCancel: onCancel
        )
        ctrl.present()
    }

    private func handleUpdateResult(_ info: (tag: String, pageURL: URL?, assetURL: URL?), manual: Bool) {
        if isVersion(info.tag, newerThan: currentAppVersion),
           let local = newestDownloadedCopy(), !isVersion(info.tag, newerThan: local.version) {
            availableUpdateVersion = info.tag
            availableUpdateURL = info.pageURL
            availableAssetURL = info.assetURL
            if !manual && CatSettings.shared.suppressAutoUpdatePrompt { return }
            // Present non-modally above the overlay window so right-click menus and quitting remain responsive.
            presentNonModalUpdatePrompt(
                title: loc("你已经下载了新版本 v\(local.version)", "You already downloaded v\(local.version)", "Você já baixou a versão v\(local.version)"),
                message: loc("但现在运行的还是旧版 v\(currentAppVersion)：\n\(Bundle.main.bundlePath)\n\n新版本位置：\n\(local.url.path)\n\n点击「切换到新版本」会关闭旧版并打开新版。点击「暂不更新」后续将不再自动弹窗提醒。",
                             "But the old v\(currentAppVersion) is still running:\n\(Bundle.main.bundlePath)\n\nNew version location:\n\(local.url.path)\n\n\"Switch to New Version\" closes the old copy and opens the new one. \"Not Now\" stops these automatic reminders.",
                             "Mas a versão antiga v\(currentAppVersion) ainda está em execução:\n\(Bundle.main.bundlePath)\n\nLocal da nova versão:\n\(local.url.path)\n\n\"Usar a nova versão\" fecha a antiga e abre a nova. \"Agora não\" desativa estes lembretes automáticos."),
                primaryTitle: loc("切换到新版本", "Switch to New Version", "Usar a nova versão"),
                cancelTitle: loc("暂不更新", "Not Now", "Agora não"),
                onPrimary: { [weak self] in
                    CatSettings.shared.suppressAutoUpdatePrompt = false
                    self?.switchToDownloadedCopy(local.url)
                },
                onCancel: {
                    CatSettings.shared.suppressAutoUpdatePrompt = true
                }
            )
            return
        }
        if isVersion(info.tag, newerThan: currentAppVersion) {
            availableUpdateVersion = info.tag
            availableUpdateURL = info.pageURL
            availableAssetURL = info.assetURL
            if !manual && CatSettings.shared.suppressAutoUpdatePrompt { return }
            promptUpdateAvailable(latestTag: info.tag)
        } else if manual {
            showInfoAlert(title: loc("已是最新版本", "You're Up to Date", "Você está atualizado"),
                          message: loc("当前版本 v\(currentAppVersion) 已是最新版本。",
                                       "v\(currentAppVersion) is the latest version.",
                                       "A versão v\(currentAppVersion) é a mais recente."))
        }
    }

    private func isVersion(_ lhs: String, newerThan rhs: String) -> Bool {
        func parse(_ v: String) -> [Int] {
            let trimmed = v.trimmingCharacters(in: CharacterSet(charactersIn: "vV "))
            return trimmed.split(separator: ".").map { Int($0.prefix(while: { $0.isNumber })) ?? 0 }
        }
        let a = parse(lhs)
        let b = parse(rhs)
        let count = max(a.count, b.count)
        for i in 0..<count {
            let va = i < a.count ? a[i] : 0
            let vb = i < b.count ? b[i] : 0
            if va != vb { return va > vb }
        }
        return false
    }

    private func promptUpdateAvailable(latestTag: String) {
        presentNonModalUpdatePrompt(
            title: loc("发现 KuroCat 新版本 \(latestTag)", "KuroCat \(latestTag) Is Available", "Nova versão do KuroCat: \(latestTag)"),
            message: loc("当前版本：v\(currentAppVersion)\n最新版本：\(latestTag)\n当前运行位置：\(Bundle.main.bundlePath)\n\n点击「立即更新」将在后台自动下载安装包并重启应用。\n点击「暂不更新」后续将不再自动弹窗提醒（随时可在菜单栏点击「检查更新」）。",
                         "Current version: v\(currentAppVersion)\nLatest version: \(latestTag)\nRunning from: \(Bundle.main.bundlePath)\n\n\"Update Now\" downloads the installer in the background and restarts the app.\n\"Not Now\" stops automatic reminders (you can still use Check for Updates in the menu).",
                         "Versão atual: v\(currentAppVersion)\nVersão mais recente: \(latestTag)\nExecutando de: \(Bundle.main.bundlePath)\n\n\"Atualizar agora\" baixa o instalador em segundo plano e reinicia o app.\n\"Agora não\" desativa os lembretes automáticos (o item Verificar atualizações do menu continua disponível)."),
            primaryTitle: loc("立即更新", "Update Now", "Atualizar agora"),
            secondaryTitle: loc("打开下载页", "Open Download Page", "Abrir página de download"),
            cancelTitle: loc("暂不更新", "Not Now", "Agora não"),
            onPrimary: { [weak self] in
                CatSettings.shared.suppressAutoUpdatePrompt = false
                self?.performInAppUpdate()
            },
            onSecondary: { [weak self] in
                let fallback = URL(string: Self.updatePageURLString)!
                NSWorkspace.shared.open(self?.availableUpdateURL ?? fallback)
            },
            onCancel: {
                CatSettings.shared.suppressAutoUpdatePrompt = true
            }
        )
    }

    private func performInAppUpdate() {
        guard !isUpdating else { return }
        guard let assetURL = availableAssetURL ?? URL(string: Self.defaultMacAssetURLString) else { return }
        isUpdating = true

        var req = URLRequest(url: assetURL, cachePolicy: .reloadIgnoringLocalCacheData, timeoutInterval: 60)
        req.setValue("KuroCat/\(currentAppVersion)", forHTTPHeaderField: "User-Agent")

        URLSession.shared.downloadTask(with: req) { [weak self] tempURL, response, error in
            guard let self else { return }
            let httpStatus = (response as? HTTPURLResponse)?.statusCode ?? 0
            guard error == nil, let tempURL, httpStatus == 200 || httpStatus == 0 else {
                DispatchQueue.main.async {
                    self.isUpdating = false
                    self.handleUpdateDownloadFailure()
                }
                return
            }

            let fm = FileManager.default
            let workDir = fm.temporaryDirectory.appendingPathComponent("KuroCatUpdate-\(UUID().uuidString)")
            let zipURL = workDir.appendingPathComponent("KuroCat-macOS-universal.zip")
            let extractDir = workDir.appendingPathComponent("extracted")
            do {
                try fm.createDirectory(at: extractDir, withIntermediateDirectories: true)
                try fm.copyItem(at: tempURL, to: zipURL)
                let attrs = try fm.attributesOfItem(atPath: zipURL.path)
                let fileSize = (attrs[.size] as? NSNumber)?.intValue ?? 0
                guard fileSize > 50_000 else {
                    throw NSError(domain: "KuroCatUpdate", code: 1)
                }

                let unzip = Process()
                unzip.executableURL = URL(fileURLWithPath: "/usr/bin/ditto")
                unzip.arguments = ["-x", "-k", zipURL.path, extractDir.path]
                try unzip.run()
                unzip.waitUntilExit()
                guard unzip.terminationStatus == 0 else {
                    throw NSError(domain: "KuroCatUpdate", code: 2)
                }

                let newAppURL = extractDir.appendingPathComponent("KuroCat.app")
                guard fm.fileExists(atPath: newAppURL.appendingPathComponent("Contents/MacOS/KuroCat").path) else {
                    throw NSError(domain: "KuroCatUpdate", code: 3)
                }

                let currentAppURL = Bundle.main.bundleURL
                let parentWritable = fm.isWritableFile(atPath: currentAppURL.deletingLastPathComponent().path)

                DispatchQueue.main.async {
                    self.installDownloadedApp(newAppURL: newAppURL, currentAppURL: currentAppURL, workDir: workDir, needsAdmin: !parentWritable)
                }
            } catch {
                try? fm.removeItem(at: workDir)
                DispatchQueue.main.async {
                    self.isUpdating = false
                    self.handleUpdateDownloadFailure()
                }
            }
        }.resume()
    }

    private func installDownloadedApp(newAppURL: URL, currentAppURL: URL, workDir: URL, needsAdmin: Bool) {
        func shQuote(_ s: String) -> String {
            "'" + s.replacingOccurrences(of: "'", with: "'\\''") + "'"
        }
        let pid = ProcessInfo.processInfo.processIdentifier
        let dst = shQuote(currentAppURL.path)
        let src = shQuote(newAppURL.path)
        let tmp = shQuote(workDir.path)

        if needsAdmin {
            let cmd = "rm -rf \(dst).bak && mv \(dst) \(dst).bak && cp -R \(src) \(dst) && rm -rf \(dst).bak \(tmp)"
            let escaped = cmd.replacingOccurrences(of: "\\", with: "\\\\").replacingOccurrences(of: "\"", with: "\\\"")
            let osa = Process()
            osa.executableURL = URL(fileURLWithPath: "/usr/bin/osascript")
            osa.arguments = ["-e", "do shell script \"\(escaped)\" with administrator privileges"]
            do {
                try osa.run()
                osa.waitUntilExit()
                if osa.terminationStatus == 0 {
                    InputEngine.shared.flushStats()
                    let relaunch = Process()
                    relaunch.executableURL = URL(fileURLWithPath: "/usr/bin/open")
                    relaunch.arguments = ["-n", currentAppURL.path]
                    try? relaunch.run()
                    NSApp.terminate(nil)
                    return
                }
            } catch {}
            try? FileManager.default.removeItem(at: workDir)
            isUpdating = false
            handleUpdateDownloadFailure()
            return
        }

        let script = """
        while kill -0 \(pid) 2>/dev/null; do sleep 0.2; done
        rm -rf \(dst).bak
        if mv \(dst) \(dst).bak && cp -R \(src) \(dst); then
            rm -rf \(dst).bak
            open -n \(dst)
        else
            mv \(dst).bak \(dst) 2>/dev/null
        fi
        rm -rf \(tmp)
        """
        let proc = Process()
        proc.executableURL = URL(fileURLWithPath: "/bin/sh")
        proc.arguments = ["-c", script]
        do {
            InputEngine.shared.flushStats()
            try proc.run()
            NSApp.terminate(nil)
        } catch {
            try? FileManager.default.removeItem(at: workDir)
            isUpdating = false
            handleUpdateDownloadFailure()
        }
    }

    private func handleUpdateDownloadFailure() {
        let alert = NSAlert()
        alert.alertStyle = .warning
        alert.messageText = loc("自动更新失败", "Automatic Update Failed", "Falha na atualização automática")
        alert.informativeText = loc("下载或替换安装包时出错，是否打开下载页手动下载？",
                                    "Something went wrong downloading or replacing the app. Open the download page to update manually?",
                                    "Houve um erro ao baixar ou substituir o app. Abrir a página de download para atualizar manualmente?")
        alert.addButton(withTitle: loc("打开下载页", "Open Download Page", "Abrir página de download"))
        alert.addButton(withTitle: loc("取消", "Cancel", "Cancelar"))
        if runModalAlert(alert) == .alertFirstButtonReturn {
            let fallback = URL(string: Self.updatePageURLString)!
            NSWorkspace.shared.open(availableUpdateURL ?? fallback)
        }
    }

    private func showInfoAlert(title: String, message: String) {
        let alert = NSAlert()
        alert.alertStyle = .informational
        alert.messageText = title
        alert.informativeText = message
        alert.addButton(withTitle: loc("好", "OK", "OK"))
        runModalAlert(alert)
    }

    // MARK: - Crisp Vector Menu Bar Cat Silhouette Icon

    private func makeMenuBarCatIcon() -> NSImage {
        let size = NSSize(width: 18, height: 18)
        let img = NSImage(size: size, flipped: true) { _ in
            guard let ctx = NSGraphicsContext.current?.cgContext else { return false }
            ctx.setFillColor(NSColor.black.cgColor)

            // Cute cat head silhouette with ears
            let path = CGMutablePath()
            path.move(to: CGPoint(x: 3.0, y: 8.5))
            path.addLine(to: CGPoint(x: 3.5, y: 2.5))
            path.addLine(to: CGPoint(x: 7.2, y: 6.0))
            path.addQuadCurve(to: CGPoint(x: 10.8, y: 6.0), control: CGPoint(x: 9.0, y: 5.5))
            path.addLine(to: CGPoint(x: 14.5, y: 2.5))
            path.addLine(to: CGPoint(x: 15.0, y: 8.5))
            path.addCurve(to: CGPoint(x: 9.0, y: 15.5), control1: CGPoint(x: 16.0, y: 12.8), control2: CGPoint(x: 13.0, y: 15.5))
            path.addCurve(to: CGPoint(x: 3.0, y: 8.5), control1: CGPoint(x: 5.0, y: 15.5), control2: CGPoint(x: 2.0, y: 12.8))
            path.closeSubpath()

            // Punch out cute cat eyes
            path.addEllipse(in: CGRect(x: 5.4, y: 8.8, width: 2.4, height: 2.2))
            path.addEllipse(in: CGRect(x: 10.2, y: 8.8, width: 2.4, height: 2.2))

            ctx.addPath(path)
            ctx.fillPath(using: .evenOdd)
            return true
        }
        img.isTemplate = true
        return img
    }

    // MARK: - Hourly Announcement

    private func startHourlyAnnouncement() {
        let calendar = Calendar.current
        let now = Date()
        let components = calendar.dateComponents([.second], from: now)
        let secondsUntilNextMinute = Double(60 - (components.second ?? 0))
        DispatchQueue.main.asyncAfter(deadline: .now() + secondsUntilNextMinute) { [weak self] in
            guard let self else { return }
            self.checkAndAnnounceHourly()
            self.minuteTimer = Timer.scheduledTimer(withTimeInterval: 60, repeats: true) { [weak self] _ in
                self?.checkAndAnnounceHourly()
            }
            RunLoop.main.add(self.minuteTimer!, forMode: .common)
        }
    }

    private func checkAndAnnounceHourly() {
        guard CatSettings.shared.hourlyAnnouncement else { return }
        let cal = Calendar.current
        let now = Date()
        let hour = cal.component(.hour, from: now)
        let minute = cal.component(.minute, from: now)
        let second = cal.component(.second, from: now)
        if minute == 0 && second < 5 && hour != lastAnnouncedHour {
            lastAnnouncedHour = hour
            showHourlyToast(hour: hour)
        }
    }

    private func showHourlyToast(hour: Int) {
        if CatSettings.shared.language != "zh" {
            let h12 = hour % 12 == 0 ? 12 : hour % 12
            let text = CatSettings.shared.language == "pt"
                ? "🕐 \(String(format: "%02d", hour)):00 em ponto"
                : "🕐 \(h12) o'clock \(hour < 12 ? "AM" : "PM")"
            overlayWindow.catView.showToast(text: text)
            wakeToActive(immediateRedraw: true)
            return
        }
        let period: String
        let displayHour: Int
        if hour == 0 {
            period = "凌晨"; displayHour = 12
        } else if hour < 6 {
            period = "凌晨"; displayHour = hour
        } else if hour == 12 {
            period = "中午"; displayHour = 12
        } else if hour < 12 {
            period = "上午"; displayHour = hour
        } else {
            period = hour < 18 ? "下午" : "晚上"
            displayHour = hour - 12
        }
        let text = "🕐 \(period) \(displayHour) 点整"
        overlayWindow.catView.showToast(text: text)
        wakeToActive(immediateRedraw: true)
    }
}

/// Non-modal update prompt kept strictly above the overlay window so buttons stay clickable
/// and right-click context menus / quitting remain completely responsive.
private final class UpdatePromptWindowController: NSWindowController {
    private var primaryAction: (() -> Void)?
    private var secondaryAction: (() -> Void)?
    private var cancelAction: (() -> Void)?

    private final class FlippedContentView: NSView {
        override var isFlipped: Bool { true }
    }

    init() {
        let win = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 440, height: 220),
            styleMask: [.titled, .closable],
            backing: .buffered,
            defer: false
        )
        win.title = loc("KuroCat 更新提醒", "KuroCat Update", "Atualização do KuroCat")
        win.isReleasedWhenClosed = false
        super.init(window: win)
        syncWindowLevel()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    func syncWindowLevel() {
        guard let window else { return }
        window.level = CatSettings.shared.fullScreenOverlay
            ? NSWindow.Level(rawValue: Int(CGWindowLevelForKey(.popUpMenuWindow)) + 2)
            : .modalPanel
        window.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]
    }

    func configure(
        title: String,
        message: String,
        primaryTitle: String,
        secondaryTitle: String?,
        cancelTitle: String,
        onPrimary: @escaping () -> Void,
        onSecondary: (() -> Void)?,
        onCancel: @escaping () -> Void
    ) {
        primaryAction = onPrimary
        secondaryAction = onSecondary
        cancelAction = onCancel

        guard let window else { return }
        let width: CGFloat = 440
        let textX: CGFloat = 86
        let textW: CGFloat = width - textX - 22

        let content = FlippedContentView(frame: NSRect(x: 0, y: 0, width: width, height: 220))

        let iconView = NSImageView(frame: NSRect(x: 20, y: 20, width: 52, height: 52))
        iconView.image = NSApp.applicationIconImage
        iconView.imageScaling = .scaleProportionallyUpOrDown
        content.addSubview(iconView)

        let titleField = NSTextField(wrappingLabelWithString: title)
        titleField.font = .systemFont(ofSize: 14, weight: .bold)
        titleField.textColor = .labelColor
        let titleH = ceil(titleField.sizeThatFits(NSSize(width: textW, height: 200)).height)
        titleField.frame = NSRect(x: textX, y: 20, width: textW, height: titleH)
        content.addSubview(titleField)

        let msgField = NSTextField(wrappingLabelWithString: message)
        msgField.font = .systemFont(ofSize: 12.5)
        msgField.textColor = .labelColor
        msgField.isSelectable = true
        let msgH = ceil(msgField.sizeThatFits(NSSize(width: textW, height: 600)).height)
        msgField.frame = NSRect(x: textX, y: 20 + titleH + 8, width: textW, height: msgH)
        content.addSubview(msgField)

        let btnY = max(88, 20 + titleH + 8 + msgH + 18)
        var rightEdge = width - 20

        func addButton(title: String, keyEquivalent: String, action: Selector) {
            let btn = NSButton(title: title, target: self, action: action)
            btn.bezelStyle = .rounded
            btn.keyEquivalent = keyEquivalent
            btn.sizeToFit()
            let bw = max(90, ceil(btn.frame.width) + 18)
            rightEdge -= bw
            btn.frame = NSRect(x: rightEdge, y: btnY, width: bw, height: 28)
            content.addSubview(btn)
            rightEdge -= 8
        }

        addButton(title: primaryTitle, keyEquivalent: "\r", action: #selector(handlePrimary))
        if let secondaryTitle {
            addButton(title: secondaryTitle, keyEquivalent: "", action: #selector(handleSecondary))
        }
        addButton(title: cancelTitle, keyEquivalent: "\u{1b}", action: #selector(handleCancel))

        let totalH = btnY + 28 + 18
        content.frame = NSRect(x: 0, y: 0, width: width, height: totalH)
        window.contentView = content
        window.setContentSize(NSSize(width: width, height: totalH))
    }

    func present() {
        syncWindowLevel()
        window?.center()
        NSApp.activate(ignoringOtherApps: true)
        showWindow(nil)
        window?.makeKeyAndOrderFront(nil)
        window?.orderFrontRegardless()
    }

    @objc private func handlePrimary() {
        window?.orderOut(nil)
        primaryAction?()
    }

    @objc private func handleSecondary() {
        window?.orderOut(nil)
        secondaryAction?()
    }

    @objc private func handleCancel() {
        window?.orderOut(nil)
        cancelAction?()
    }
}
