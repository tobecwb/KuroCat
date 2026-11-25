<div align="center">

# 🐱 KuroCat

**Ultra-Lightweight Native Desktop Cat Pet · Full-State Keyboard & Mouse Overlay (macOS / Windows / Linux / UOS)**

<p>
  <a href="https://github.com/chanke17/KuroCat/releases/latest"><img src="https://img.shields.io/github/v/release/chanke17/KuroCat?style=flat-square&color=1f6feb&label=Release" alt="Latest Release"/></a>
  <a href="https://github.com/chanke17/KuroCat/releases"><img src="https://img.shields.io/github/downloads/chanke17/KuroCat/total?style=flat-square&color=2ea44f&label=Downloads" alt="Total Downloads"/></a>
  <a href="https://github.com/chanke17/KuroCat/stargazers"><img src="https://img.shields.io/github/stars/chanke17/KuroCat?style=flat-square&color=e3b341&label=Stars" alt="GitHub Stars"/></a>
  <img src="https://img.shields.io/badge/Platform-macOS%20%7C%20Windows%20%7C%20Linux%20%7C%20UOS-555555?style=flat-square" alt="Platforms"/>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/chanke17/KuroCat?style=flat-square&color=8957e5" alt="MIT License"/></a>
  <a href="https://signpath.io/"><img src="https://img.shields.io/badge/Code%20Signing-SignPath.io-0098ff?style=flat-square" alt="Code Signing by SignPath.io"/></a>
</p>

<p>
  <a href="README.md">Português (BR)</a> | <a href="README.zh-CN.md">简体中文</a> | <b>English</b> ·
  <a href="https://kurocat.cc/en/">🌐 Official Site (Live Demo)</a> ·
  <a href="https://workshop.kurocat.cc/">🎨 Skin Workshop &amp; Plaza</a> ·
  <a href="https://kurocat.cc/obs-guide/">🎥 OBS Overlay Guide</a> ·
  <a href="https://github.com/chanke17/KuroCat/releases/latest">⬇️ Download Latest</a>
</p>

<p>
  <img src="assets/demo_typing.gif" width="265" alt="Typing on keyboard"/>
  <img src="assets/demo_hunting.gif" width="265" alt="Hunting boba eyes on fast mouse movement"/>
  <img src="assets/demo_combo.gif" width="265" alt="Typing + dragging mouse + 3rd key with hind paw"/>
</p>

<sub>Native macOS (Swift + CoreGraphics + IOHIDManager) · Windows (C++17 + GDI+ + Raw Input) · Linux / UOS (C++17 + GTK3/Cairo + XInput2/evdev)</sub>

</div>

---

## 💡 Why KuroCat?

| Feature | 🐱 KuroCat | Electron / Web-based Pets |
| :--- | :--- | :--- |
| **Binary Size** | **~802 KB** (single dependency-free Windows `.exe`) | 80 MB – 150 MB+ (bundled Chromium) |
| **Resource Usage** | Native Swift / C++17, adaptive 30/20/10 fps idle throttling | Heavy background renderer process |
| **In-Game Input** | **True NKRO hardware capture**, works in DirectX / Vulkan games | Often blocked by IME, game focus, or UAC |
| **2.5D Animation** | Live2D-style spherical head tracking + hunting boba eyes + long-cat drag | Static sprite swaps |
| **Skins & AI** | Online [2.5D Skin Workshop](https://workshop.kurocat.cc/) + [AI MCP Server](mcp/README.md) | Manual image/asset editing |
| **Privacy** | **100% local in-memory processing**, zero keystroke logging | Closed-source or telemetry-heavy |

---

## ✨ Highlights

- **🎮 Full-State Input Capture for Typing & Gaming (True NKRO)**
  - **macOS**: Dual-layer `IOHIDManager` (kernel HID layer) + `CGEventTap`, unaffected by IME switching. Supports holding `WASD`, combos, standalone modifiers (`⌘ / ⇧ / ⌥ / ⌃ / Fn`), arrow keys, and `F1–F12`.
  - **Windows**: Native `Raw Input API (RIDEV_INPUTSINK)` hardware scan-code capture—tracks keystrokes and mouse micro-movements even inside DirectX/Vulkan games.
  - **Linux & UOS / Kylin**: Dual-layer `XInput2` + kernel `evdev`. **Works out of the box without `sudo` on UOS / Deepin / Ubuntu X11**; on Wayland, add your user to the `input` group.
- **🌀 2.5D Spherical Head Tracking & Hunting Boba Eyes**
  - Head, ears, eyes, and whiskers rotate with realistic 2.5D spherical perspective toward your cursor. Keyboard and mouse face the cat 180°, matching your natural left-hand keyboard / right-hand mouse posture.
  - Shake your mouse fast to trigger **Hunting Mode**: pupils dilate into huge round boba eyes with highlights, crouching low and wagging its tail faster.
- **🐾 Fun Interactive Details**
  - **3-Key Hind Paw & 4-Key Tail Press**: Hold 3 or 4 keys simultaneously and the cat lifts its hind paw or curls its tail to press the extra key!
  - **Keyboard Marquee Backlight**: A comet-like RGB chase light sweeps diagonally across the keyboard—faster and longer with more keys held (individually toggleable).
  - **Petting & Sleep**: Stroke the cat's head for cozy closed eyes `— —` and shimmer particles; leave it idle to fall asleep with floating `z`s.
  - **Click-Through & Launch at Login**: Transparent areas pass clicks through automatically; toggle **Full Click-Through** for competitive gaming, **Launch at Login**, and auto-update notifications.
- **🎨 Skins, Themes & Stats**
  - Built-in skins (Classic Black, Siamese, American Shorthair, Orange & White, Brown Mackerel Tabby) plus community `.kurocat` skins & `.kurokb` keyboards.
  - 3 keyboard themes, 6+ eye colors, 5 scale levels, daily keystroke counter, and live KPM stats.

<details>
<summary><b>🖼️ Built-in Skin 6-State Previews (Click to expand)</b></summary>
<br/>

| Classic Black Cat | Siamese Cat |
| :---: | :---: |
| <img src="assets/preview_black.jpg" width="420" alt="Classic Black Cat preview"/> | <img src="assets/preview_siamese.jpg" width="420" alt="Siamese Cat preview"/> |

| American Shorthair | Orange & White Cat |
| :---: | :---: |
| <img src="assets/preview_shorthair.jpg" width="420" alt="American Shorthair preview"/> | <img src="assets/preview_orangeWhite.jpg" width="420" alt="Orange & White Cat preview"/> |

<p align="center">
  <img src="assets/preview_tabby_windows.png" width="520" alt="Classic Black and Brown Mackerel Tabby sitting and typing"/>
</p>

</details>

---

## 📦 Download & Run

Grab the latest pre-built binaries from **[GitHub Releases](https://github.com/chanke17/KuroCat/releases/latest)**:

### 🍎 macOS (Universal: Apple Silicon & Intel · macOS 11+)
1. Download `KuroCat-macOS-universal.zip`, unzip, and drag `KuroCat.app` into **Applications** (menu bar app, no Dock icon).
2. Click the bottom prompt pill to enable `KuroCat` in **System Settings → Privacy & Security → Input Monitoring**, then click **Restart KuroCat** from the menu bar.
3. While **Full Click-Through** is active, use the menu bar icon or **hold `⌥ Option` to right-click / drag**.

<details>
<summary>🛠️ Build from source on macOS</summary>

```bash
chmod +x build.sh
./build.sh
open KuroCat.app
```

</details>

### 🪟 Windows (Windows 7 SP1 / 8 / 10 / 11 x64)
1. Download the single-file `KuroCat-Windows-x64.exe` (~**802 KB**, zero external runtime dependencies). Double-click to run; lives in the system tray.
2. While **Full Click-Through** is active, exit or open the menu anytime via:
   - Global hotkey: **`Ctrl + Alt + Shift + P`**;
   - Temporary unlock: hold **`Alt`** and right-click / drag the cat;
   - System tray icon, or **double-click `KuroCat.exe` again** to reset click-through and open the menu.

<details>
<summary>❓ Windows FAQ & Build from Source</summary>

- **Follows mouse but not keyboard in a game?** Games running as Administrator isolate keystrokes from normal apps. Right-click the cat → **Restart as administrator** (or enable **Always run as administrator**).
- **Cat disappears in-game?** Switch the game from *Exclusive Fullscreen* to *Borderless / Windowed Fullscreen*.
- **Still prompted to update after downloading?** An older process was still running; newer builds auto-close older instances on launch.

**Build from source (MSVC / MinGW / CMake):**
```bat
cd windows
build.bat
:: Or via CMake
cmake -B build && cmake --build build --config Release
```

</details>

### 🐧 Linux & UOS / Kylin (x86_64 & ARM64)
Supports **Ubuntu 20.04+ / Debian 11+ / Deepin / UOS / Kylin / Arch / Fedora**:

1. **No-password install (recommended on UOS / Kylin)**: extract `KuroCat-Linux-x86_64.tar.gz` or `KuroCat-Linux-arm64.tar.gz` and run `bash install.sh` (installs to `~/.local` without `sudo`; `bash uninstall.sh` removes it).
2. **`.deb` package**:
   ```bash
   sudo apt install ./KuroCat-Linux-amd64.deb
   ```

<details>
<summary>❓ Linux / Wayland / Build from Source</summary>

- **Wayland sessions**: Run `sudo usermod -aG input $USER` once and re-login to enable kernel-level `evdev` capture.
- **Kylin blocks `.deb` install?** Set *Security Center → App Protection / Execution Control* to Warn or Off, or use `install.sh`.
- **Build from source (including LoongArch)**:
  ```bash
  sudo apt install g++ pkg-config libgtk-3-dev libcairo2-dev libx11-dev libxi-dev
  cd linux && chmod +x build.sh build_deb.sh
  ./build.sh        # Compile binary
  ./build_deb.sh    # Package .deb installer
  ```

</details>

---

## 🌐 Ecosystem & Tools

| Resource | Link | Description |
| :--- | :--- | :--- |
| **Official Website** | [kurocat.cc/en/](https://kurocat.cc/en/) · [中文](https://kurocat.cc/) | Interactive 2.5D web pet demo, SHA-256 verified downloads & FAQ |
| **Skin Workshop & Plaza** | [workshop.kurocat.cc](https://workshop.kurocat.cc/) | Browser-based 2.5D skin editor + free `.kurocat` skins & `.kurokb` keyboards |
| **OBS Streaming Guide** | [kurocat.cc/obs-guide](https://kurocat.cc/obs-guide/) | Transparent window capture, `#00FF00` green screen & gaming click-through setup |
| **KuroCat MCP Server** | [mcp/README.md](mcp/README.md) | Zero-dependency Node.js MCP server for Claude, Cursor & Antigravity to generate and install skins |
| **Skin Format Spec** | [docs/skin-format.md](docs/skin-format.md) | `.kurocat` v1 schema reference & coordinate diagram |

---

## 🤝 Contributing

- **Submit Skins & Keyboards**: Open a PR on [KuroCat-Workshop](https://github.com/chanke17/KuroCat-Workshop) to publish your `.kurocat` or `.kurokb` creations to the Community Plaza.
- **Code & Bug Reports**: Contributions are welcome via [Issues](https://github.com/chanke17/KuroCat/issues) and Pull Requests!

<details>
<summary><b>📁 Project Structure</b></summary>

```text
KuroCat/
├── Sources/                    # macOS native Swift source (Universal)
├── Resources/                  # macOS Info.plist & AppIcon.icns
├── build.sh                    # macOS universal build script
├── windows/                    # Windows native C++17 (Win32 + GDI+ + Raw Input)
├── linux/                      # Linux & UOS native C++17 (GTK3 + Cairo + XInput2 + evdev)
├── mcp/                        # Zero-dependency Node.js MCP Server
├── docs/                       # Skin format specification & screenshots
└── .github/workflows/build.yml # Multi-platform automated release workflow
```

</details>

---

## 🔒 Privacy & Code Signing

- **Privacy**: All keyboard and mouse signals are processed **strictly in local memory in real time**—no keystrokes are ever logged or saved. Aside from checking for updates, KuroCat makes zero network requests.
- **Code Signing Policy**: Free code signing provided by [SignPath.io](https://signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).
  - **Committers & Reviewers**: Project maintainers of `chanke17/KuroCat` (reproducibly built via GitHub Actions).
  - **Approvers**: Repository owner (`chanke17`).

## ⭐ Star History

<a href="https://star-history.com/#chanke17/KuroCat&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=chanke17/KuroCat&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=chanke17/KuroCat&type=Date" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=chanke17/KuroCat&type=Date" width="600" />
 </picture>
</a>

## 📄 License

Released under the [MIT License](LICENSE).
