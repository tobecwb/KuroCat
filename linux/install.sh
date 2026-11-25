#!/usr/bin/env bash
# KuroCat 免密码安装：只安装给当前用户（~/.local），不需要 sudo / 开机密码。
# 适用于 统信 UOS、银河麒麟 Kylin、深度 Deepin、Ubuntu、Debian 等。
# 用法：在解压后的文件夹里右键「在终端中打开」，执行：bash install.sh
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}"
BIN_DIR="$HOME/.local/bin"
APP_DIR="$DATA_HOME/applications"
ICON_DIR="$DATA_HOME/icons/hicolor/scalable/apps"

if [ ! -f "$HERE/kurocat" ]; then
    echo "找不到 kurocat 程序文件，请在解压后的 KuroCat 文件夹里运行本脚本。" >&2
    exit 1
fi

# 先确认这台电脑能运行（CPU 架构 / 系统 glibc 版本），避免装完了才发现打不开
if ! VERSION_OUT="$("$HERE/kurocat" --version 2>&1)"; then
    echo "这台电脑无法运行当前的 KuroCat 安装包：" >&2
    echo "  $VERSION_OUT" >&2
    echo "" >&2
    echo "常见原因：" >&2
    echo "  1. CPU 架构不匹配：当前电脑是 $(uname -m)。x86_64/兆芯/海光 请下载 x86_64 包，" >&2
    echo "     飞腾/鲲鹏/麒麟 ARM 电脑 请下载 arm64 包；龙芯（loongarch64/mips64）暂不支持。" >&2
    echo "  2. 缺少图形库：请在应用商店或终端安装 libgtk-3-0 和 libxi6。" >&2
    exit 1
fi

mkdir -p "$BIN_DIR" "$APP_DIR" "$ICON_DIR"

# 先关掉正在运行的旧版小猫（新版本启动时也会自动接管，这里只是更干净）
pkill -x kurocat >/dev/null 2>&1 || true

install -m 755 "$HERE/kurocat" "$BIN_DIR/kurocat.new"
mv -f "$BIN_DIR/kurocat.new" "$BIN_DIR/kurocat"
[ -f "$HERE/kurocat.svg" ] && install -m 644 "$HERE/kurocat.svg" "$ICON_DIR/kurocat.svg"

# 写入「开始菜单 / 启动器」图标（同名文件会覆盖系统里旧的 .deb 版本入口）
if [ -f "$HERE/kurocat.desktop" ]; then
    sed "s|^Exec=.*|Exec=\"$BIN_DIR/kurocat\"|" "$HERE/kurocat.desktop" > "$APP_DIR/kurocat.desktop"
else
    cat > "$APP_DIR/kurocat.desktop" <<EOF
[Desktop Entry]
Name=KuroCat
Name[zh_CN]=KuroCat 黑猫键盘桌宠
Name[pt_BR]=KuroCat – gatinho de teclado
Exec="$BIN_DIR/kurocat"
Icon=kurocat
Terminal=false
Type=Application
Categories=Utility;Amusement;
EOF
fi
chmod 644 "$APP_DIR/kurocat.desktop"
command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$APP_DIR" >/dev/null 2>&1 || true
command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -q "$DATA_HOME/icons/hicolor" >/dev/null 2>&1 || true

echo "KuroCat 已安装到 $BIN_DIR/kurocat（$VERSION_OUT），无需管理员密码。"
echo "以后可以在「启动器 / 开始菜单」里搜索 KuroCat 打开；卸载请运行 bash uninstall.sh"

# 立即启动
nohup "$BIN_DIR/kurocat" >/dev/null 2>&1 &
