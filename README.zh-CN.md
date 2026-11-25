<div align="center">

# 🐱 KuroCat

**超轻量级全状态键鼠桌面萌猫 · 原生跨平台 (macOS / Windows / Linux / 统信 UOS)**

<p>
  <a href="https://github.com/chanke17/KuroCat/releases/latest"><img src="https://img.shields.io/github/v/release/chanke17/KuroCat?style=flat-square&color=1f6feb&label=Release" alt="Latest Release"/></a>
  <a href="https://github.com/chanke17/KuroCat/releases"><img src="https://img.shields.io/github/downloads/chanke17/KuroCat/total?style=flat-square&color=2ea44f&label=Downloads" alt="Total Downloads"/></a>
  <a href="https://github.com/chanke17/KuroCat/stargazers"><img src="https://img.shields.io/github/stars/chanke17/KuroCat?style=flat-square&color=e3b341&label=Stars" alt="GitHub Stars"/></a>
  <img src="https://img.shields.io/badge/Platform-macOS%20%7C%20Windows%20%7C%20Linux%20%7C%20UOS-555555?style=flat-square" alt="Platforms"/>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/chanke17/KuroCat?style=flat-square&color=8957e5" alt="MIT License"/></a>
  <a href="https://signpath.io/"><img src="https://img.shields.io/badge/Code%20Signing-SignPath.io-0098ff?style=flat-square" alt="Code Signing by SignPath.io"/></a>
</p>

<p>
  <a href="README.md">Português (BR)</a> | <b>简体中文</b> | <a href="README.en.md">English</a> ·
  <a href="https://kurocat.cc/">🌐 官方首页（网页试玩）</a> ·
  <a href="https://workshop.kurocat.cc/">🎨 皮肤工坊 &amp; 广场</a> ·
  <a href="https://kurocat.cc/obs-guide/">🎥 OBS 直播指南</a> ·
  <a href="https://github.com/chanke17/KuroCat/releases/latest">⬇️ GitHub 下载</a> ·
  <a href="https://kurocat-download.app.workbuddy.host/?sharecode=JVAABIVZ7L">⚡ 国内高速下载</a>
</p>

<p>
  <img src="assets/demo_typing.gif" width="265" alt="敲键盘打字"/>
  <img src="assets/demo_hunting.gif" width="265" alt="快速晃鼠标捕猎大圆眼"/>
  <img src="assets/demo_combo.gif" width="265" alt="边打字边拖鼠标 + 3键用脚按"/>
</p>

<sub>Native macOS (Swift + CoreGraphics + IOHIDManager) · Windows (C++17 + GDI+ + Raw Input) · Linux / 统信 UOS (C++17 + GTK3/Cairo + XInput2/evdev)</sub>

</div>

---

## 💡 为什么选择 KuroCat？

| 对比维度 | 🐱 KuroCat | 传统 Electron / Web 桌宠 |
| :--- | :--- | :--- |
| **安装包体积** | **~802 KB**（Windows 单文件零依赖） | 80 MB ~ 150 MB+（捆绑 Chromium） |
| **内存 / CPU 占用** | 原生 Swift / C++17，空闲自适应降帧至 10fps | 常驻渲染进程，内存占用高 |
| **游戏内按键捕获** | **硬件级真全键无冲 (NKRO)**，支持 DirectX / Vulkan 游戏 | 易受输入法、游戏焦点或管理员权限拦截 |
| **2.5D 视觉表现** | Live2D 同款球面投影转头 + 捕猎大圆眼 + 长猫拖拽 | 多为静态贴图切换 |
| **皮肤与生态** | 在线 [2.5D 皮肤工坊](https://workshop.kurocat.cc/) + [AI MCP Server](mcp/README.md) | 改图门槛高，需手动替换资源 |
| **隐私安全** | **100% 本地内存实时处理**，绝不记录按键与写日志 | 闭源软件难以审计 |

---

## ✨ 核心特性

- **🎮 打字 & 游戏全状态捕获（真全键无冲 NKRO）**
  - **macOS**：双层 `IOHIDManager`（内核 HID 硬件层）+ `CGEventTap`，不受中英文输入法干扰，完整支持 `WASD` 长按、组合键、独立修饰键（`⌘ / ⇧ / ⌥ / ⌃ / Fn`）、方向键与 `F1–F12`。
  - **Windows**：原生 `Raw Input API (RIDEV_INPUTSINK)` 硬件扫描码监听，支持 DirectX/Vulkan 游戏内按键与锁视角鼠标微动。
  - **Linux & 统信 UOS / 麒麟**：双层 `XInput2` + 内核 `evdev`，**UOS / Deepin / Ubuntu X11 下双击即用、免 `sudo`**；纯 Wayland 加入 `input` 组即可开启硬件级监听。
- **🌀 2.5D 球面转头 & 捕猎大圆眼**
  - 猫头、双耳、双眼与胡须随鼠标光标产生真实的 2.5D 透视转头；键盘与鼠标 180° 朝向小猫摆放，左手键盘右手鼠标自然不交叉。
  - 快速晃动鼠标触发**捕猎模式**：瞳孔扩张为超大圆润黑瞳 + 白色高光，身子压低伏击、尾巴加快抽动。
- **🐾 趣味交互彩蛋**
  - **3 键「用脚按」 & 4 键「用尾巴按」**：同时按下第 3、第 4 个键时，小猫会翘起后脚或甩出尾巴去按！
  - **键盘跑马灯流光**：彗星追光沿键盘对角线扫过，按键越多速度越快、尾迹越长，色相每圈轮转（可在右键菜单单独开关）。
  - **摸头互动 & 打瞌睡**：轻撸猫头触发惬意闭眼 `— —` 与微光粒子，闲置久了自动打瞌睡冒 `z`。
  - **游戏防误触穿透 & 开机自启**：透明区域自动穿透，支持「完全鼠标穿透」、开机自启与自动更新检测。
- **🎨 皮肤工坊、多配色 & 统计**
  - 内置多款原生皮肤（经典小黑猫 / 挖煤暹罗猫 / 美短 / 橘白猫 / 狸花猫），支持导入社区 `.kurocat` 皮肤与 `.kurokb` 键盘。
  - 3 套键盘配色、6+ 种瞳色、5 档尺寸缩放，自带今日击键数与实时 KPM 统计。

<details>
<summary><b>🖼️ 查看内置皮肤 6 态效果预览（点击展开）</b></summary>
<br/>

| 经典小黑猫 | 挖煤暹罗猫 |
| :---: | :---: |
| <img src="assets/preview_black.jpg" width="420" alt="经典小黑猫预览"/> | <img src="assets/preview_siamese.jpg" width="420" alt="挖煤暹罗猫预览"/> |

| 美短（银虎斑加白） | 橘白猫 |
| :---: | :---: |
| <img src="assets/preview_shorthair.jpg" width="420" alt="美短六态预览"/> | <img src="assets/preview_orangeWhite.jpg" width="420" alt="橘白猫六态预览"/> |

<p align="center">
  <img src="assets/preview_tabby_windows.png" width="520" alt="原版小黑猫与狸花猫对比"/>
</p>

</details>

---

## 📦 快速安装与使用

前往 **[GitHub Releases](https://github.com/chanke17/KuroCat/releases/latest)**（或 [国内高速直链](https://kurocat-download.app.workbuddy.host/?sharecode=JVAABIVZ7L)）下载对应平台安装包：

### 🍎 macOS (Apple Silicon & Intel 通用 · macOS 11+)
1. 下载 `KuroCat-macOS-universal.zip` 解压，将 `KuroCat.app` 拖入「应用程序」打开（常驻顶部菜单栏，不占 Dock）。
2. 首次使用按底部提示前往 **系统设置 → 隐私与安全性 → 输入监控** 勾选 `KuroCat`，在菜单栏点「重启 KuroCat」即可。
3. 开启「完全鼠标穿透」后，可通过菜单栏图标或 **按住 `⌥ Option` 右键/拖动小猫**。

<details>
<summary>🛠️ macOS 本地源码编译</summary>

```bash
chmod +x build.sh
./build.sh
open KuroCat.app
```

</details>

### 🪟 Windows (Windows 7 SP1 / 8 / 10 / 11 x64)
1. 下载单文件 `KuroCat-Windows-x64.exe`（约 **802 KB**，零运行时依赖），双击即用，常驻右下角系统托盘。
2. 开启「完全鼠标穿透」后可通过以下任意方式操作：
   - 全局快捷键 **`Ctrl + Alt + Shift + P`** 一键切换穿透；
   - 按住 **`Alt`** 键临时解锁（可右键菜单或左键拖动）；
   - 点击任务栏托盘区小猫图标，或 **再次双击 `KuroCat.exe`** 自动解除穿透并弹出菜单。

<details>
<summary>❓ Windows 常见问题 & 本地编译</summary>

- **游戏里只跟鼠标不跟键盘？** 游戏以管理员权限运行时会被系统拦截输入，右键小猫 →「以管理员身份重新启动」（或勾选「每次启动都以管理员身份运行」）。
- **进游戏小猫消失？** 若游戏使用「独占全屏」，请改为「无边框窗口 / 窗口化全屏」，小猫会自动重新置顶。
- **下载了新版仍提示更新？** 旧版仍在后台运行，新版启动时会自动接管，也可按提示路径删除旧版 `.exe`。

**本地编译（MSVC / MinGW / CMake）：**
```bat
cd windows
build.bat
:: 或使用 CMake
cmake -B build && cmake --build build --config Release
```

</details>

### 🐧 Linux & 统信 UOS / 银河麒麟 (x86_64 & ARM64)
支持 **Ubuntu 20.04+ / Debian 11+ / Deepin / 统信 UOS / 银河麒麟 / Arch / Fedora**：

1. **免密码安装（推荐 UOS / 麒麟）**：下载 `KuroCat-Linux-x86_64.tar.gz` 或 `KuroCat-Linux-arm64.tar.gz` 解压，执行 `bash install.sh`（安装至 `~/.local`，无需 sudo；卸载执行 `bash uninstall.sh`）。
2. **`.deb` 安装包**：
   ```bash
   sudo apt install ./KuroCat-Linux-amd64.deb
   ```

<details>
<summary>❓ Linux / Wayland / 龙芯源码编译说明</summary>

- **Wayland 会话**：执行一次 `sudo usermod -aG input $USER` 并重新登录即可启用 `evdev` 内核级监听。
- **麒麟安装被拦截**：「安全中心 → 应用保护 / 应用执行控制」设为「警告」或「关闭」，或改用免密码 `install.sh`。
- **性能优化**：默认采用省电自适应帧率（30 / 20 / 10 fps），低配设备建议保持关闭菜单中的「高帧率 60fps」。
- **源码编译（含 LoongArch / MIPS）**：
  ```bash
  sudo apt install g++ pkg-config libgtk-3-dev libcairo2-dev libx11-dev libxi-dev
  cd linux && chmod +x build.sh build_deb.sh
  ./build.sh        # 编译可执行文件
  ./build_deb.sh    # 一键打包 .deb 安装包
  ```

</details>

---

## 🌐 生态与工具

| 项目 / 入口 | 链接 | 说明 |
| :--- | :--- | :--- |
| **官方网站** | [kurocat.cc](https://kurocat.cc/) · [English](https://kurocat.cc/en/) | 网页内直接试玩 2.5D 桌宠、三端 SHA-256 校验下载 |
| **在线皮肤工坊 & 广场** | [workshop.kurocat.cc](https://workshop.kurocat.cc/) | 照片取色捏猫、贝塞尔花纹画板、免费下载社区 `.kurocat` 皮肤与 `.kurokb` 键盘 |
| **OBS 直播配置指南** | [kurocat.cc/obs-guide](https://kurocat.cc/obs-guide/) | OBS 透明捕获 / `#00FF00` 绿幕抠像、抖音直播伴侣、B 站直播姬教程 |
| **KuroCat MCP Server** | [mcp/README.md](mcp/README.md) | 零依赖 Node.js MCP Server：让 Claude / Cursor / Antigravity 直接 AI 捏猫与安装皮肤 |
| **皮肤格式规范** | [docs/skin-format.md](docs/skin-format.md) | `.kurocat` v1 完整字段规范与坐标图解 |

---

## 🤝 参与贡献

- **投稿皮肤 / 键盘**：前往 [KuroCat-Workshop](https://github.com/chanke17/KuroCat-Workshop) 提交 PR，即可上线到 [皮肤广场](https://workshop.kurocat.cc/) 供所有玩家一键下载。
- **代码与问题反馈**：欢迎提交 [Issue](https://github.com/chanke17/KuroCat/issues) 或 Pull Request 改进三端体验。

<details>
<summary><b>📁 项目目录结构</b></summary>

```text
KuroCat/
├── Sources/                    # macOS 原生 Swift 源码 (Universal)
├── Resources/                  # macOS Info.plist & AppIcon.icns
├── build.sh                    # macOS 编译脚本
├── windows/                    # Windows 原生 C++17 (Win32 + GDI+ + Raw Input)
├── linux/                      # Linux & UOS 原生 C++17 (GTK3 + Cairo + XInput2 + evdev)
├── mcp/                        # KuroCat MCP Server (Node.js 零依赖)
├── docs/                       # 皮肤格式规范与截图文档
└── .github/workflows/build.yml # 全平台自动构建与发版工作流
```

</details>

---

## 🔒 隐私与代码签名

- **隐私承诺**：KuroCat 的所有键鼠硬件信号**仅在本地内存实时驱动桌宠动画与计数**，**绝不记录按键内容、绝不写日志**；除检查更新外无任何联网行为，源码完全开源可审计。
- **Code Signing Policy**：Free code signing provided by [SignPath.io](https://signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).
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
