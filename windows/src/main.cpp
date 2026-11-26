#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif
#define NOMINMAX
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>
// MSVC's GDI+ headers call unqualified min()/max() inside inline helpers; NOMINMAX
// removes the macros, so the std overloads must be visible before <gdiplus.h>.
#include <algorithm>
using std::min;
using std::max;
#include <objidl.h>
#include <gdiplus.h>
#include <wtsapi32.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <cmath>
#include <cwchar>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "KeyMap.hpp"
#include "InputEngine.hpp"
#include "CatPhysics.hpp"
#include "CatRenderer.hpp"
#include "SkinLibrary.hpp"
#include "../../shared/UiLanguage.hpp"

// UI string for the current language (kept in sync with CatSettings::language via applyUiLanguage / startup).
inline const wchar_t* trW(const wchar_t* zh, const wchar_t* en, const wchar_t* pt) {
    return uiTextW(static_cast<int>(kurocat::uiLanguage()), zh, en, pt);
}

#ifndef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
#define WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY 4
#endif
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 0x00000200
#endif
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0x00000800
#endif

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wtsapi32.lib")

namespace {

constexpr const wchar_t* APP_VERSION = L"1.5.2";
constexpr const wchar_t* VERSION_PROP = L"KuroCatVersion"; // set on the tray owner window so newer builds can take over older ones
constexpr const wchar_t* TRAY_OWNER_CLASS = L"KuroCatTrayOwnerClass";
constexpr const wchar_t* OVERLAY_CLASS = L"KuroCatOverlayWindowClass";
constexpr double FRAME_INTERVAL = 1.0 / 60.0;
constexpr const wchar_t* FORK_RELEASES_URL = L"https://github.com/tobecwb/KuroCat/releases";
constexpr const wchar_t* UPDATE_PAGE_URL = L"https://kurocat-download.app.workbuddy.host/?sharecode=JVAABIVZ7L";
constexpr const wchar_t* UPDATE_WIN_EXE_PATH = L"/downloads/KuroCat-Windows-x64.exe?sharecode=JVAABIVZ7L";
constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT WM_WAKE_EXISTING = WM_APP + 2;
constexpr UINT WM_UPDATE_CHECK_DONE = WM_APP + 3;
constexpr UINT WM_UPDATE_DOWNLOAD_DONE = WM_APP + 4;
constexpr UINT WM_REQUEST_QUIT = WM_APP + 5; // posted by a newer / elevated instance that is taking over
constexpr UINT_PTR TIMER_RENDER = 1;
constexpr int HOTKEY_CLICKTHROUGH_ID = 101;
UINT g_msgTaskbarCreated = 0;

enum MenuCmd : UINT {
    CMD_TOGGLE_VISIBLE = 1001,
    CMD_TOGGLE_CLICKTHROUGH,
    CMD_TOGGLE_TOPMOST,
    CMD_TOGGLE_AUTOHIDE_FS,
    CMD_TOGGLE_AUTOSTART,
    CMD_TOGGLE_BADGES,
    CMD_TOGGLE_MOUSE_BADGES,
    CMD_TOGGLE_KEY_LEGENDS,
    CMD_TOGGLE_FOOT,
    CMD_TOGGLE_TAIL,
    CMD_TOGGLE_MARQUEE,
    CMD_TOGGLE_LEVEL_KEYBOARD,
    CMD_TOGGLE_PET,
    CMD_OBS_OFF,
    CMD_OBS_TRANSPARENT,
    CMD_OBS_GREENSCREEN,
    CMD_TOGGLE_OBS_HIDE,
    CMD_TOGGLE_EDIT,
    CMD_LANG_ZH,
    CMD_LANG_EN,
    CMD_LANG_PT,
    CMD_COAT_BLACK,
    CMD_COAT_SIAMESE,
    CMD_COAT_SHORTHAIR,
    CMD_COAT_ORANGE_WHITE,
    CMD_COAT_DAZED,
    CMD_KB_MILK,
    CMD_KB_HACKER,
    CMD_KB_RETRO,
    CMD_EYE_LIME,
    CMD_EYE_BLUE,
    CMD_EYE_AMBER,
    CMD_EYE_CYAN,
    CMD_EYE_SOFT_AMBER,
    CMD_EYE_REFERENCE_HAZEL,
    CMD_SCALE_65,
    CMD_SCALE_85,
    CMD_SCALE_100,
    CMD_SCALE_125,
    CMD_SCALE_150,
    CMD_DEMO_BURST,
    CMD_RESET_POS,
    CMD_CHECK_UPDATE,
    CMD_OPEN_UPDATE,
    CMD_RELAUNCH_ADMIN,
    CMD_TOGGLE_RUN_AS_ADMIN,
    CMD_QUIT,
    CMD_COAT_TABBY,
    CMD_EYE_OLIVE_GOLD,
    CMD_TABLET_AUTO,
    CMD_TABLET_ON,
    CMD_TABLET_OFF,
    CMD_IMPORT_SKIN,
    CMD_OPEN_WORKSHOP,
    CMD_OPEN_WEBSITE,
    CMD_OPEN_SKINS_DIR,
    CMD_IMPORT_KB,
    CMD_OPEN_KB_PLAZA,
    CMD_OPEN_KB_DIR,
    CMD_OPEN_SETTINGS,
    CMD_BATCH_DELETE_SKINS,
    CMD_SKIN_SELECT_BASE = 4000,  // + index into AppState::skins.skins
    CMD_SKIN_DELETE_BASE = 5000,
    CMD_KB_SELECT_BASE = 6000,    // + index into AppState::kbLibrary.skins
    CMD_KB_DELETE_BASE = 7000,
    CMD_SKIN_RENAME_BASE = 8000,
    CMD_SKIN_RANGE = 900
};
constexpr const wchar_t* DRESSING_ROOM_CLASS = L"KuroCatDressingRoomClass";
constexpr const wchar_t* SETTINGS_WINDOW_CLASS = L"KuroCatSettingsWindowClass";
constexpr const wchar_t* UPDATE_WINDOW_CLASS = L"KuroCatUpdateWindowClass";
constexpr int IDC_DRESSING_CHOOSE = 11;

struct UpdateCheckResult {
    bool ok = false;
    bool manual = false;
    bool hasUpdate = false;
    std::wstring latestTag;
    std::wstring releaseUrl;
};

struct UpdateDownloadResult {
    bool ok = false;
    std::wstring newExePath;
    std::wstring targetExePath;
};

enum class UpdateDialogKind {
    None,
    UpdateAvailable,
    UpToDate,
    CheckFailed,
    DownloadFailed
};

struct RenderedPoseSnapshot {
    int w = 0, h = 0;
    std::string skinId;
    std::string kbSkinId;
    int coatTheme = 0;
    int eyeTheme = 0;
    int tabletMode = 0;
    int obsCaptureMode = 0;
    bool flipHorizontal = false;
    int rotationDeg10 = 0;
    bool isEditMode = false;
    bool showKeyOverlay = false;
    bool showStats = false;
    int language = 2;
    bool isMusicPlaying = false;
    bool foregroundBlocksKeys = false;
    bool leftMouseDown = false;
    bool rightMouseDown = false;
    size_t activeKeyCount = 0;
    uint64_t activeKeyHash = 0;
    int totalKeystrokesToday = 0;
    int totalPetSessions = 0;
    int headX10 = 0, headY10 = 0;
    int leftPawY10 = 0, rightPawY10 = 0, thirdPawY10 = 0;
    int breath100 = 0, blink100 = 0, sleep100 = 0;
    int tail10 = 0, purr100 = 0, happy100 = 0;
    int sparkleCount = 0, zzzCount = 0, noteCount = 0;

    bool operator==(const RenderedPoseSnapshot& o) const {
        return w == o.w && h == o.h &&
               skinId == o.skinId && kbSkinId == o.kbSkinId &&
               coatTheme == o.coatTheme && eyeTheme == o.eyeTheme &&
               tabletMode == o.tabletMode && obsCaptureMode == o.obsCaptureMode &&
               flipHorizontal == o.flipHorizontal && rotationDeg10 == o.rotationDeg10 &&
               isEditMode == o.isEditMode && showKeyOverlay == o.showKeyOverlay &&
               showStats == o.showStats && language == o.language &&
               isMusicPlaying == o.isMusicPlaying && foregroundBlocksKeys == o.foregroundBlocksKeys &&
               leftMouseDown == o.leftMouseDown && rightMouseDown == o.rightMouseDown &&
               activeKeyCount == o.activeKeyCount && activeKeyHash == o.activeKeyHash &&
               totalKeystrokesToday == o.totalKeystrokesToday && totalPetSessions == o.totalPetSessions &&
               headX10 == o.headX10 && headY10 == o.headY10 &&
               leftPawY10 == o.leftPawY10 && rightPawY10 == o.rightPawY10 && thirdPawY10 == o.thirdPawY10 &&
               breath100 == o.breath100 && blink100 == o.blink100 && sleep100 == o.sleep100 &&
               tail10 == o.tail10 && purr100 == o.purr100 && happy100 == o.happy100 &&
               sparkleCount == o.sparkleCount && zzzCount == o.zzzCount && noteCount == o.noteCount;
    }
};

struct AppState {
    HWND hwnd = nullptr;
    HWND menuOwnerHwnd = nullptr; // Non-NOACTIVATE helper window so TrackPopupMenu from tray NEVER fails!
    HICON trayIcon = nullptr;
    NOTIFYICONDATAW nid = {};

    CatSettings settings;
    InputEngine input;
    CatPhysics physics;
    SkinLibrary skins;
    KeyboardSkinLibrary kbLibrary;
    HWND dressingRoom = nullptr;    // 「换装间」 import window (nullptr when closed)
    std::wstring dressingStatusText;  // status line of 换装间; kind 0 neutral, 1 success, 2 error
    int dressingStatusKind = 0;
    float dressingScale = 1.0f;       // DPI scale of the 换装间 layout (design units are 96-dpi pixels)
    HWND settingsWnd = nullptr;     // unified native Settings window (nullptr when closed)
    HFONT settingsFont = nullptr;
    HFONT settingsBoldFont = nullptr;
    HBRUSH settingsBgBrush = nullptr;
    bool settingsDark = false;
    bool settingsUpdating = false;

    bool isVisible = true;
    bool hiddenByFullscreen = false;
    bool isDragging = false;
    int activeResizeCorner = -1; // -1 = none, 0=TL, 1=TR, 2=BL, 3=BR
    POINT resizeAnchorScreenPt = {};
    bool isRotating = false;
    bool rotateMoved = false;
    float rotateStartAngle = 0.0f;
    float rotateStartDeg = 0.0f;
    bool rButtonDown = false;
    bool pendingMenuAfterAlt = false;
    double pendingMenuAltUpTime = 0.0;
    POINT pendingMenuPt = {};
    bool inContextMenu = false;
    POINT dragStartCursor = {};
    POINT dragStartWinOrigin = {};
    bool hasLastHover = false;
    Pt lastHoverCanvas = {};
    std::string statsDay;
    int lastSavedPetSessions = 0;

    // Reusable 32-bit premultiplied DIB backing buffer
    HDC memDC = nullptr;
    HBITMAP dibBmp = nullptr;
    HBITMAP oldBmp = nullptr;
    void* dibBits = nullptr;
    int bufW = 0;
    int bufH = 0;

    double lastTickTime = 0.0;
    bool demoActive = false;
    double demoStartTime = 0.0;
    int demoStepIdx = 0;

    std::wstring availableUpdateVersion;
    std::wstring availableUpdateUrl;
    bool isUpdating = false;
    bool isManualUpdateCheck = false;
    HWND updateWnd = nullptr;
    HFONT updateFont = nullptr;
    HFONT updateBoldFont = nullptr;
    HFONT updateHeadingFont = nullptr;
    HBRUSH updateBgBrush = nullptr;
    bool updateDark = false;
    bool updateIsWarning = false;
    int updateDefaultBtnId = IDOK;
    bool replacingUpdateWnd = false;
    UpdateDialogKind updateDialogKind = UpdateDialogKind::None;
    std::wstring updateDialogUrl;
    UpdateCheckResult* pendingUpdateCheck = nullptr;
    UpdateDownloadResult* pendingUpdateDownload = nullptr;

    // Robustness against games, Win+L, sleep and display changes
    double nextFrameTime = 0.0;
    double lastRenderTime = 0.0;
    double nextHousekeepingTime = 0.0;
    double pendingTopmostTime = 0.0;
    int ulwFailures = 0;
    bool isElevated = false;
    bool foregroundBlocksKeys = false;   // foreground app runs as admin -> Windows (UIPI) hides its keys from us
    double lastBlockedForegroundTime = 0.0;
    double blockedForegroundSince = 0.0;
    bool warnedElevatedForeground = false;
    HWINEVENTHOOK foregroundHook = nullptr;

    // Idle/dirty rendering & background WASAPI polling
    bool forceRedraw = true;
    RenderedPoseSnapshot lastRenderedPose;
    std::atomic<bool> audioPollingEnabled{false};
    std::atomic<bool> audioSustainedPlaying{false};
    std::atomic<bool> audioThreadStop{false};
    std::mutex audioThreadMu;
    std::condition_variable audioThreadCv;
    std::thread audioThread;
    bool sessionNotifyRegistered = false;

    void destroyBuffer() {
        if (memDC && oldBmp) SelectObject(memDC, oldBmp);
        if (dibBmp) DeleteObject(dibBmp);
        if (memDC) DeleteDC(memDC);
        memDC = nullptr; dibBmp = nullptr; oldBmp = nullptr; dibBits = nullptr;
        bufW = 0; bufH = 0;
    }

    void ensureBuffer(int w, int h) {
        if (w == bufW && h == bufH && memDC && dibBits) return;
        destroyBuffer();

        HDC screenDC = GetDC(nullptr);
        memDC = CreateCompatibleDC(screenDC);
        ReleaseDC(nullptr, screenDC);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h; // top-down 32bpp DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        dibBmp = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &dibBits, nullptr, 0);
        if (dibBmp) {
            oldBmp = (HBITMAP)SelectObject(memDC, dibBmp);
            bufW = w;
            bufH = h;
        }
    }
};

AppState* g_app = nullptr;

HICON createCatTrayIcon();

// MARK: - Windows Auto-Start (HKCU\Software\Microsoft\Windows\CurrentVersion\Run)

std::wstring getCurrentExeQuotedPath() {
    wchar_t buf[MAX_PATH * 2] = {};
    DWORD len = GetModuleFileNameW(nullptr, buf, (DWORD)(sizeof(buf) / sizeof(buf[0]) - 1));
    if (len == 0) return L"";
    return L"\"" + std::wstring(buf, len) + L"\"";
}

bool isAutoStartEnabled() {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    DWORD type = 0, size = 0;
    bool exists = (RegQueryValueExW(hKey, L"KuroCat", nullptr, &type, nullptr, &size) == ERROR_SUCCESS && size > 0);
    RegCloseKey(hKey);
    return exists;
}

bool setAutoStartEnabled(bool enable) {
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    bool ok = false;
    if (enable) {
        std::wstring cmd = getCurrentExeQuotedPath();
        if (!cmd.empty()) {
            ok = (RegSetValueExW(hKey, L"KuroCat", 0, REG_SZ, (const BYTE*)cmd.c_str(), (DWORD)((cmd.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS);
        }
    } else {
        LSTATUS st = RegDeleteValueW(hKey, L"KuroCat");
        ok = (st == ERROR_SUCCESS || st == ERROR_FILE_NOT_FOUND);
    }
    RegCloseKey(hKey);
    return ok;
}

void showTrayBalloon(AppState& app, const wchar_t* title, const wchar_t* body) {
    NOTIFYICONDATAW b = app.nid;
    b.uFlags = NIF_INFO;
    b.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
    wcsncpy_s(b.szInfoTitle, title, _TRUNCATE);
    wcsncpy_s(b.szInfo, body, _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &b);
}

// MARK: - Elevation (UIPI) helpers
//
// Many games (and launchers such as WeGame / anti-cheat clients) run as administrator. Windows'
// User Interface Privilege Isolation then refuses to deliver that window's keystrokes to normal
// programs — Raw Input, GetAsyncKeyState and low-level hooks all go silent — while mouse movement
// is still visible. That is exactly the "cat only sees the mouse in games" report. The only fix
// is for KuroCat to run at the same level, so we detect it and offer an admin relaunch.

bool isCurrentProcessElevated() {
    HANDLE tok = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) return false;
    TOKEN_ELEVATION te = {};
    DWORD sz = 0;
    bool elevated = GetTokenInformation(tok, TokenElevation, &te, sizeof(te), &sz) && te.TokenIsElevated != 0;
    CloseHandle(tok);
    return elevated;
}

bool isProcessElevatedOrProtected(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return GetLastError() == ERROR_ACCESS_DENIED; // protected / anti-cheat processes
    bool elevated = false;
    HANDLE tok = nullptr;
    if (OpenProcessToken(h, TOKEN_QUERY, &tok)) {
        TOKEN_ELEVATION te = {};
        DWORD sz = 0;
        if (GetTokenInformation(tok, TokenElevation, &te, sizeof(te), &sz)) elevated = te.TokenIsElevated != 0;
        CloseHandle(tok);
    } else {
        elevated = (GetLastError() == ERROR_ACCESS_DENIED);
    }
    CloseHandle(h);
    return elevated;
}

bool commandLineHasFlag(const wchar_t* flag) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return false;
    bool found = false;
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], flag) == 0) { found = true; break; }
    }
    LocalFree(argv);
    return found;
}

/// Starts a new elevated copy of this exe (UAC prompt). Returns false if the user declined.
bool relaunchSelfElevated(const wchar_t* params) {
    wchar_t exe[MAX_PATH * 2] = {};
    if (GetModuleFileNameW(nullptr, exe, (DWORD)(sizeof(exe) / sizeof(exe[0]) - 1)) == 0) return false;
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOASYNC;
    sei.lpVerb = L"runas";
    sei.lpFile = exe;
    sei.lpParameters = params;
    sei.nShow = SW_SHOWNORMAL;
    return ShellExecuteExW(&sei) != FALSE;
}

/// True when the foreground window covers its whole monitor (WPS PPT slideshow, fullscreen video, games).
bool isForegroundFullscreen(HWND selfHwnd) {
    HWND fg = GetForegroundWindow();
    if (!fg || fg == selfHwnd || fg == GetDesktopWindow() || fg == GetShellWindow()) return false;
    wchar_t cls[64] = {};
    GetClassNameW(fg, cls, 63);
    if (wcscmp(cls, L"WorkerW") == 0 || wcscmp(cls, L"Progman") == 0 ||
        wcscmp(cls, L"Shell_TrayWnd") == 0 || wcscmp(cls, L"Shell_SecondaryTrayWnd") == 0) return false;
    RECT wr = {};
    if (!GetWindowRect(fg, &wr)) return false;
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST), &mi)) return false;
    return wr.left <= mi.rcMonitor.left + 2 && wr.top <= mi.rcMonitor.top + 2 &&
           wr.right >= mi.rcMonitor.right - 2 && wr.bottom >= mi.rcMonitor.bottom - 2;
}

bool hasVisibleWindowAbove(HWND hwnd) {
    if (!hwnd) return false;
    HWND prev = GetWindow(hwnd, GW_HWNDPREV);
    int steps = 0;
    while (prev && steps++ < 16) {
        if (IsWindowVisible(prev)) {
            RECT r = {};
            if (GetWindowRect(prev, &r) && (r.right - r.left) > 4 && (r.bottom - r.top) > 4) {
                return true;
            }
        }
        prev = GetWindow(prev, GW_HWNDPREV);
    }
    return false;
}

// MARK: - Auto Update Checker (GitHub Releases via WinHTTP)

std::string extractJsonStringField(const std::string& json, const char* fieldName) {
    std::string key = std::string("\"") + fieldName + "\"";
    size_t pos = json.find(key);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + key.size());
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

bool isSemVerNewer(const std::wstring& lhs, const std::wstring& rhs) {
    auto parse = [](const std::wstring& v) {
        std::vector<int> parts;
        size_t i = 0;
        while (i < v.size() && (v[i] == L'v' || v[i] == L'V' || v[i] == L' ')) i++;
        int cur = 0; bool inNum = false;
        for (; i < v.size(); ++i) {
            if (v[i] >= L'0' && v[i] <= L'9') {
                cur = cur * 10 + (v[i] - L'0');
                inNum = true;
            } else if (v[i] == L'.') {
                parts.push_back(inNum ? cur : 0);
                cur = 0; inNum = false;
            } else {
                break;
            }
        }
        if (inNum) parts.push_back(cur);
        return parts;
    };
    auto a = parse(lhs);
    auto b = parse(rhs);
    size_t n = std::max(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        int va = i < a.size() ? a[i] : 0;
        int vb = i < b.size() ? b[i] : 0;
        if (va != vb) return va > vb;
    }
    return false;
}

/// "1.3.3" -> 10303. Stored as a window property so a newly launched (newer) build can tell
/// which version is already running. Builds up to 1.3.2 never set it, which reads back as 0.
int versionToNumber(const std::wstring& v) {
    int parts[3] = { 0, 0, 0 };
    int idx = 0;
    for (wchar_t c : v) {
        if (c >= L'0' && c <= L'9') parts[idx] = parts[idx] * 10 + (c - L'0');
        else if (c == L'.') { if (++idx >= 3) break; }
    }
    return parts[0] * 10000 + parts[1] * 100 + parts[2];
}

std::wstring currentExePath() {
    wchar_t buf[MAX_PATH * 2] = {};
    DWORD len = GetModuleFileNameW(nullptr, buf, (DWORD)(sizeof(buf) / sizeof(buf[0]) - 1));
    return std::wstring(buf, len);
}

std::string extractVersionFromHtml(const std::string& html) {
    auto parseVerAt = [&](size_t pos) -> std::string {
        if (pos >= html.size() || (html[pos] != 'v' && html[pos] != 'V')) return "";
        size_t i = pos + 1;
        int dots = 0;
        std::string ver = "v";
        while (i < html.size()) {
            char c = html[i];
            if (c >= '0' && c <= '9') {
                ver.push_back(c);
            } else if (c == '.') {
                dots++;
                ver.push_back(c);
            } else {
                break;
            }
            i++;
        }
        return (dots >= 2 && ver.size() > 5) ? ver : "";
    };

    const char* marker = "\xE5\xB7\xB2\xE5\x8F\x91\xE5\xB8\x83"; // "已发布" in UTF-8
    size_t mPos = html.find(marker);
    if (mPos != std::string::npos) {
        size_t searchStart = (mPos > 32) ? (mPos - 32) : 0;
        for (size_t i = mPos; i > searchStart; --i) {
            if (html[i - 1] == 'v' || html[i - 1] == 'V') {
                std::string v = parseVerAt(i - 1);
                if (!v.empty()) return v;
            }
        }
    }

    const char* verPrefix = "\xE7\x89\x88\xE6\x9C\xAC "; // "版本 " in UTF-8
    size_t vPos = html.find(verPrefix);
    if (vPos != std::string::npos) {
        std::string v = parseVerAt(vPos + 7);
        if (!v.empty()) return v;
    }
    return "";
}

std::string fetchWorkBuddyPath(HINTERNET hConnect, LPCWSTR path) {
    std::string body;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (hRequest) {
        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, nullptr)) {
            DWORD statusCode = 0, statusSize = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
            if (statusCode >= 200 && statusCode < 300) {
                DWORD avail = 0;
                while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0) {
                    std::vector<char> buf(avail);
                    DWORD readBytes = 0;
                    if (WinHttpReadData(hRequest, buf.data(), avail, &readBytes) && readBytes > 0) {
                        body.append(buf.data(), readBytes);
                    } else {
                        break;
                    }
                }
            }
        }
        WinHttpCloseHandle(hRequest);
    }
    return body;
}

HINTERNET openWinHttpSession(const std::wstring& ua) {
    // Windows 8.1+ supports AUTOMATIC_PROXY; Windows 7 requires DEFAULT_PROXY.
    HINTERNET hSession = WinHttpOpen(ua.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        hSession = WinHttpOpen(ua.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    }
    if (hSession) {
        // Enable TLS 1.2 explicitly for Windows 7 SP1 (KB3140245) compatibility
        DWORD protos = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1;
        WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &protos, sizeof(protos));
    }
    return hSession;
}

void enableBestDpiAwareness() {
    // 1. Windows 10 1703+: Per-Monitor V2 DPI Awareness (dynamically loaded so Win7/8 won't fail to launch)
    if (HMODULE hUser32 = GetModuleHandleW(L"user32.dll")) {
        using SetDpiCtxFn = BOOL(WINAPI*)(HANDLE);
        auto fn = reinterpret_cast<SetDpiCtxFn>(GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));
        if (fn && fn(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4)))) {
            return;
        }
    }
    // 2. Windows 8.1+: Per-Monitor DPI Awareness via shcore.dll
    if (HMODULE hShcore = LoadLibraryW(L"shcore.dll")) {
        using SetDpiAwareFn = HRESULT(WINAPI*)(int);
        auto fn = reinterpret_cast<SetDpiAwareFn>(GetProcAddress(hShcore, "SetProcessDpiAwareness"));
        bool ok = (fn && SUCCEEDED(fn(2)));
        FreeLibrary(hShcore);
        if (ok) return;
    }
    // 3. Windows Vista / 7: System DPI Awareness
    if (HMODULE hUser32 = GetModuleHandleW(L"user32.dll")) {
        using SetSysDpiFn = BOOL(WINAPI*)();
        auto fn = reinterpret_cast<SetSysDpiFn>(GetProcAddress(hUser32, "SetProcessDPIAware"));
        if (fn) fn();
    }
}

struct UpdateCheckThreadParams {
    HWND notifyHwnd;
    bool manual;
    int delayMs;
};

DWORD WINAPI updateCheckThreadProc(LPVOID param) {
    auto* p = static_cast<UpdateCheckThreadParams*>(param);
    HWND notifyHwnd = p->notifyHwnd;
    bool manual = p->manual;
    int delayMs = p->delayMs;
    delete p;

    if (delayMs > 0) Sleep((DWORD)delayMs);
    auto* res = new UpdateCheckResult();
    res->manual = manual;

    std::wstring ua = std::wstring(L"KuroCat/") + APP_VERSION;
    HINTERNET hSession = openWinHttpSession(ua);
    if (hSession) {
        WinHttpSetTimeouts(hSession, 8000, 8000, 8000, 8000);
        HINTERNET hConnect = WinHttpConnect(hSession, L"kurocat-download.app.workbuddy.host", INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (hConnect) {
            std::string body = fetchWorkBuddyPath(hConnect, L"/version.json?sharecode=JVAABIVZ7L");
            std::string tag = extractJsonStringField(body, "tag_name");
            std::string url = extractJsonStringField(body, "html_url");

            if (tag.empty()) {
                std::string html = fetchWorkBuddyPath(hConnect, L"/?sharecode=JVAABIVZ7L");
                tag = extractVersionFromHtml(html);
            }

            if (!tag.empty()) {
                res->ok = true;
                res->latestTag = utf8ToWide(tag);
                res->releaseUrl = url.empty() ? UPDATE_PAGE_URL : utf8ToWide(url);
                res->hasUpdate = isSemVerNewer(res->latestTag, APP_VERSION);
            }
            WinHttpCloseHandle(hConnect);
        }
        WinHttpCloseHandle(hSession);
    }

    if (!PostMessageW(notifyHwnd, WM_UPDATE_CHECK_DONE, 0, reinterpret_cast<LPARAM>(res))) {
        delete res;
    }
    return 0;
}

bool startAsyncUpdateCheck(HWND notifyHwnd, bool manual, int delayMs = 0) {
    auto* params = new UpdateCheckThreadParams{ notifyHwnd, manual, delayMs };
    HANDLE hThread = CreateThread(nullptr, 0, updateCheckThreadProc, params, 0, nullptr);
    if (hThread) {
        CloseHandle(hThread);
        return true;
    }
    delete params;
    return false;
}

bool launchProcessArgs(const std::wstring& exePath, const std::wstring& args) {
    std::wstring cmdLine = L"\"" + exePath + L"\"";
    if (!args.empty()) {
        cmdLine += L" ";
        cmdLine += args;
    }
    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(L'\0');
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessW(
        exePath.c_str(),
        cmdBuf.data(),
        nullptr, nullptr, FALSE,
        0, nullptr, nullptr,
        &si, &pi
    );
    if (ok) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }
    return false;
}

DWORD WINAPI updateDownloadThreadProc(LPVOID param) {
    HWND notifyHwnd = reinterpret_cast<HWND>(param);
    auto* res = new UpdateDownloadResult();
    std::string exeBytes;

    std::wstring ua = std::wstring(L"KuroCat/") + APP_VERSION;
    HINTERNET hSession = openWinHttpSession(ua);
    if (hSession) {
        WinHttpSetTimeouts(hSession, 10000, 10000, 30000, 60000);
        HINTERNET hConnect = WinHttpConnect(hSession, L"kurocat-download.app.workbuddy.host", INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (hConnect) {
            exeBytes = fetchWorkBuddyPath(hConnect, UPDATE_WIN_EXE_PATH);
            WinHttpCloseHandle(hConnect);
        }
        WinHttpCloseHandle(hSession);
    }

    if (exeBytes.size() > 50000 && exeBytes[0] == 'M' && exeBytes[1] == 'Z') {
        wchar_t curExe[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, curExe, MAX_PATH);
        wchar_t tempDir[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, tempDir);

        std::wstring curExeStr(curExe);
        std::wstring newExePath = (curExeStr.size() > 4 && _wcsicmp(curExeStr.c_str() + curExeStr.size() - 4, L".exe") == 0)
            ? curExeStr.substr(0, curExeStr.size() - 4) + L".update.exe"
            : curExeStr + L".update.exe";
        HANDLE hNew = CreateFileW(newExePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hNew == INVALID_HANDLE_VALUE) {
            newExePath = std::wstring(tempDir) + L"KuroCat_update.exe";
            hNew = CreateFileW(newExePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        }
        if (hNew != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            BOOL okWrite = WriteFile(hNew, exeBytes.data(), (DWORD)exeBytes.size(), &written, nullptr);
            CloseHandle(hNew);
            if (okWrite && written == (DWORD)exeBytes.size()) {
                res->ok = true;
                res->newExePath = newExePath;
                res->targetExePath = curExeStr;
            }
        }
    }

    if (!PostMessageW(notifyHwnd, WM_UPDATE_DOWNLOAD_DONE, 0, reinterpret_cast<LPARAM>(res))) {
        delete res;
    }
    return 0;
}

void startInAppUpdate(AppState& app) {
    if (app.isUpdating || !app.menuOwnerHwnd) return;
    app.isUpdating = true;
    HANDLE hThread = CreateThread(nullptr, 0, updateDownloadThreadProc, app.menuOwnerHwnd, 0, nullptr);
    if (hThread) {
        CloseHandle(hThread);
    } else {
        app.isUpdating = false;
    }
}

const wchar_t* trayTipText() {
    return trW(L"KuroCat 黑猫键盘桌宠 (右键/左键打开菜单)",
               L"KuroCat Keyboard Cat (click to open the menu)",
               L"KuroCat – gatinho do teclado (clique para abrir o menu)");
}

void registerTrayIcon(AppState& app) {
    if (!app.trayIcon) app.trayIcon = createCatTrayIcon();
    app.nid = {};
    app.nid.cbSize = sizeof(app.nid);
    app.nid.hWnd = app.menuOwnerHwnd ? app.menuOwnerHwnd : app.hwnd;
    app.nid.uID = 1;
    app.nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    app.nid.uCallbackMessage = WM_TRAYICON;
    app.nid.hIcon = app.trayIcon;
    wcscpy_s(app.nid.szTip, trayTipText());
    Shell_NotifyIconW(NIM_ADD, &app.nid);
}

HICON createCatTrayIcon() {
    using namespace Gdiplus;
    constexpr int sz = 32;
    Bitmap bmp(sz, sz, PixelFormat32bppPARGB);
    Graphics g(&bmp);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.Clear(Color(0, 0, 0, 0));

    SolidBrush black(Color(255, 22, 22, 26));
    PointF lEar[3] = { {4, 15}, {6, 3}, {14, 11} };
    PointF rEar[3] = { {28, 15}, {26, 3}, {18, 11} };
    g.FillPolygon(&black, lEar, 3);
    g.FillPolygon(&black, rEar, 3);
    g.FillEllipse(&black, 4.0f, 8.0f, 24.0f, 21.0f);

    SolidBrush lime(Color(255, 201, 242, 64));
    g.FillEllipse(&lime, 8.5f, 14.5f, 6.0f, 5.2f);
    g.FillEllipse(&lime, 17.5f, 14.5f, 6.0f, 5.2f);
    g.FillEllipse(&black, 10.7f, 15.2f, 1.8f, 3.8f);
    g.FillEllipse(&black, 19.7f, 15.2f, 1.8f, 3.8f);

    HICON hIcon = nullptr;
    bmp.GetHICON(&hIcon);
    return hIcon;
}

void computeDefaultOrigin(int w, int h, int& outX, int& outY) {
    RECT work = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    outX = work.right - w - 24;
    outY = work.bottom - h - 12;
}

void applyWindowScaleAndPosition(AppState& app, bool anchorBottomCenter, bool persist = true) {
    int newW = (int)std::round(CatPhysics::CanvasWidth * app.settings.scale);
    int newH = (int)std::round(CatPhysics::CanvasHeight * app.settings.scale);

    int x = app.settings.posX;
    int y = app.settings.posY;
    if (anchorBottomCenter && app.hwnd) {
        RECT r = {};
        GetWindowRect(app.hwnd, &r);
        int oldMidX = (r.left + r.right) / 2;
        int oldBotY = r.bottom;
        x = oldMidX - newW / 2;
        y = oldBotY - newH;
    } else if (x == -99999 || y == -99999) {
        computeDefaultOrigin(newW, newH, x, y);
    }

    // Clamp onto virtual desktop so saved off-monitor coords never get lost
    int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (x + newW < vx + 40 || x > vx + vw - 40 || y + newH < vy + 40 || y > vy + vh - 40) {
        computeDefaultOrigin(newW, newH, x, y);
    }

    bool updateBlockingTopmost = (app.updateWnd && IsWindow(app.updateWnd) && IsWindowVisible(app.updateWnd) && !IsIconic(app.updateWnd));
    bool wantTopmost = app.settings.alwaysOnTop
        && !(app.settings.obsCaptureMode > 0 && app.settings.obsHideDesktop)
        && !(app.dressingRoom && IsWindowVisible(app.dressingRoom) && !IsIconic(app.dressingRoom))
        && !updateBlockingTopmost;
    HWND insertAfter = wantTopmost ? HWND_TOPMOST : (updateBlockingTopmost ? app.updateWnd : HWND_NOTOPMOST);
    if (updateBlockingTopmost && (GetWindowLongPtrW(app.hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)) {
        SetWindowPos(app.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
    SetWindowPos(app.hwnd, insertAfter, x, y, newW, newH, SWP_NOACTIVATE);
    if (persist) {
        app.settings.posX = x;
        app.settings.posY = y;
        app.settings.saveToRegistry();
    }
}

bool shouldCatBeTopmost(const AppState& app) {
    if (!app.settings.alwaysOnTop) return false;
    if (app.settings.obsCaptureMode > 0 && app.settings.obsHideDesktop) return false;
    if (app.dressingRoom && IsWindowVisible(app.dressingRoom) && !IsIconic(app.dressingRoom)) return false;
    if (app.updateWnd && IsWindow(app.updateWnd) && IsWindowVisible(app.updateWnd) && !IsIconic(app.updateWnd)) return false;
    return true;
}

/// Puts the cat back on top of other topmost windows (borderless/fullscreen games that make
/// themselves topmost after launching used to bury the cat, which looked like it "vanished").
void assertTopmost(AppState& app) {
    if (!app.hwnd || !app.isVisible || app.hiddenByFullscreen) return;
    if (!shouldCatBeTopmost(app)) return;
    SetWindowPos(app.hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
}

void demoteTopmostForUpdateDialog(AppState& app) {
    if (app.hwnd && IsWindow(app.hwnd) && (GetWindowLongPtrW(app.hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)) {
        SetWindowPos(app.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
    if (app.dressingRoom && IsWindow(app.dressingRoom) && (GetWindowLongPtrW(app.dressingRoom, GWL_EXSTYLE) & WS_EX_TOPMOST)) {
        SetWindowPos(app.dressingRoom, HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
}

void restoreTopmostAfterUpdateDialog(AppState& app) {
    if (app.hwnd && IsWindow(app.hwnd) && app.isVisible && !app.hiddenByFullscreen && shouldCatBeTopmost(app)) {
        assertTopmost(app);
    }
    if (app.dressingRoom && IsWindow(app.dressingRoom) && IsWindowVisible(app.dressingRoom)) {
        SetWindowPos(app.dressingRoom, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
}

/// Full self-repair after Win+L unlock, sleep/resume, display-mode changes (games switching
/// resolution) or a rendering stall: clears stuck keys, re-registers Raw Input, rebuilds the
/// layered-window buffer, restarts the frame timer and re-places the window on a visible monitor
/// WITHOUT overwriting the user's saved position.
void applyOBSMode(AppState& app);

void recoverAfterSystemEvent(AppState& app) {
    if (!app.hwnd) return;
    double now = nowSeconds();
    app.input.resetAllState();
    app.input.registerRawInput(app.hwnd);
    app.input.installKeyboardHook(app.hwnd);
    if (app.settings.obsCaptureMode > 0 && app.settings.obsHideDesktop) {
        applyOBSMode(app);
    }
    app.input.lastMouseMoveTime = now; // wake the cat up so it visibly greets the user again
    app.destroyBuffer();
    app.ulwFailures = 0;
    app.isDragging = false;
    app.lastTickTime = now;
    app.nextFrameTime = 0.0;
    app.lastRenderTime = now;
    KillTimer(app.hwnd, TIMER_RENDER);
    SetTimer(app.hwnd, TIMER_RENDER, 16, nullptr);
    if (app.isVisible && !app.hiddenByFullscreen) ShowWindow(app.hwnd, SW_SHOWNOACTIVATE);
    applyWindowScaleAndPosition(app, false, false);
    app.pendingTopmostTime = now + 1.5; // some games re-assert topmost shortly after a mode switch
}

/// Returns true when any active audio output endpoint (speakers, 3.5mm/USB/Bluetooth headphones)
/// currently has a non-system application outputting audible audio (videos, browsers, Apple Music,
/// music players, etc.). Uses WASAPI endpoint enumeration + AudioSessionState + IAudioMeterInformation,
/// with a safe fallback for Win11 Bluetooth A2DP / hardware-offloaded endpoints whose GetPeakValue()
/// reports 0.0f while audio is actively playing.
static bool isSystemAudioPlaying() {
    static std::unordered_set<std::wstring> s_meterWorkingDevices;

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    bool tempComInit = false;
    if (hr == CO_E_NOTINITIALIZED) {
        HRESULT initHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(initHr)) {
            tempComInit = true;
            hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
        }
    }
    if (FAILED(hr) || !enumerator) {
        if (tempComInit) CoUninitialize();
        return false;
    }

    IMMDeviceCollection* collection = nullptr;
    hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
    enumerator->Release();
    if (FAILED(hr) || !collection) {
        if (tempComInit) CoUninitialize();
        return false;
    }

    UINT devCount = 0;
    collection->GetCount(&devCount);
    bool found = false;

    for (UINT d = 0; d < devCount && !found; ++d) {
        IMMDevice* device = nullptr;
        if (FAILED(collection->Item(d, &device)) || !device) continue;

        std::wstring devId;
        LPWSTR rawDevId = nullptr;
        if (SUCCEEDED(device->GetId(&rawDevId)) && rawDevId) {
            devId = rawDevId;
            CoTaskMemFree(rawDevId);
        }

        // Skip muted or zero-volume output endpoints
        bool epMuted = false;
        float epVolume = 1.0f;
        IAudioEndpointVolume* epVol = nullptr;
        if (SUCCEEDED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&epVol)) && epVol) {
            BOOL muted = FALSE;
            if (SUCCEEDED(epVol->GetMute(&muted)) && muted) epMuted = true;
            float vol = 1.0f;
            if (SUCCEEDED(epVol->GetMasterVolumeLevelScalar(&vol))) epVolume = vol;
            epVol->Release();
        }
        if (epMuted || epVolume <= 0.001f) {
            device->Release();
            continue;
        }

        // Query endpoint meter without skipping when epPeak <= 0.0005f (Win11 BT A2DP / offload
        // endpoints often return S_OK with 0.0f peak even while playing audio).
        float epPeak = 0.0f;
        bool hasEpMeter = false;
        UINT epMeterChannels = 0;
        IAudioMeterInformation* epMeter = nullptr;
        if (SUCCEEDED(device->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, nullptr, (void**)&epMeter)) && epMeter) {
            float pk = 0.0f;
            if (SUCCEEDED(epMeter->GetPeakValue(&pk))) {
                epPeak = pk;
                hasEpMeter = true;
            }
            epMeter->GetMeteringChannelCount(&epMeterChannels);
            epMeter->Release();
        }

        IAudioSessionManager2* mgr = nullptr;
        if (SUCCEEDED(device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&mgr)) && mgr) {
            IAudioSessionEnumerator* sessionEnum = nullptr;
            if (SUCCEEDED(mgr->GetSessionEnumerator(&sessionEnum)) && sessionEnum) {
                int count = 0;
                sessionEnum->GetCount(&count);
                for (int i = 0; i < count && !found; ++i) {
                    IAudioSessionControl* ctrl = nullptr;
                    if (FAILED(sessionEnum->GetSession(i, &ctrl)) || !ctrl) continue;
                    AudioSessionState st = AudioSessionStateInactive;
                    if (SUCCEEDED(ctrl->GetState(&st)) && st == AudioSessionStateActive) {
                        bool isSystemSound = false;
                        IAudioSessionControl2* ctrl2 = nullptr;
                        if (SUCCEEDED(ctrl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&ctrl2)) && ctrl2) {
                            if (ctrl2->IsSystemSoundsSession() == S_OK) isSystemSound = true;
                            ctrl2->Release();
                        }
                        bool sessionMuted = false;
                        float sessionVolume = 1.0f;
                        ISimpleAudioVolume* sVol = nullptr;
                        if (SUCCEEDED(ctrl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&sVol)) && sVol) {
                            BOOL sm = FALSE;
                            if (SUCCEEDED(sVol->GetMute(&sm)) && sm) sessionMuted = true;
                            float sv = 1.0f;
                            if (SUCCEEDED(sVol->GetMasterVolume(&sv))) sessionVolume = sv;
                            sVol->Release();
                        }
                        if (!isSystemSound && !sessionMuted && sessionVolume > 0.001f) {
                            float sPeak = 0.0f;
                            bool hasSessionPeak = false;
                            IAudioMeterInformation* sMeter = nullptr;
                            if (SUCCEEDED(ctrl->QueryInterface(__uuidof(IAudioMeterInformation), (void**)&sMeter)) && sMeter) {
                                if (SUCCEEDED(sMeter->GetPeakValue(&sPeak))) {
                                    hasSessionPeak = true;
                                }
                                sMeter->Release();
                            }
                            if (hasSessionPeak && sPeak > 0.0008f) {
                                if (!devId.empty()) s_meterWorkingDevices.insert(devId);
                                found = true;
                            } else if (hasEpMeter && epPeak > 0.001f && !hasSessionPeak) {
                                found = true;
                            } else if (!hasEpMeter || epMeterChannels == 0 || !hasSessionPeak ||
                                       devId.empty() || s_meterWorkingDevices.find(devId) == s_meterWorkingDevices.end()) {
                                // Endpoint meter has never reported non-zero session peaks (e.g. Win11 Bluetooth A2DP
                                // / hardware-offloaded stream) or metering is unavailable: trust active unmuted session.
                                found = true;
                            }
                        }
                    }
                    ctrl->Release();
                }
                sessionEnum->Release();
            }
            mgr->Release();
        }
        device->Release();
    }
    collection->Release();
    if (tempComInit) CoUninitialize();
    return found;
}

/// Twice per second: watchdog, fullscreen auto-hide, topmost re-assert, and UIPI detection.
void housekeeping(AppState& app) {
    double now = nowSeconds();
    if (now < app.nextHousekeepingTime) return;
    app.nextHousekeepingTime = now + 0.5;
    if (!app.hwnd) return;

    // Read background WASAPI polling result without blocking the UI thread
    app.audioPollingEnabled.store(app.settings.autoHeadphonesOnMusic, std::memory_order_relaxed);
    app.physics.isMusicPlaying = app.settings.autoHeadphonesOnMusic
        ? app.audioSustainedPlaying.load(std::memory_order_relaxed)
        : false;

    bool fullscreenFg = isForegroundFullscreen(app.hwnd);
    bool obsBg = (app.settings.obsCaptureMode > 0 && app.settings.obsHideDesktop);

    // Fullscreen auto-hide (PPT slideshow, fullscreen video, games)
    if (app.isVisible && app.settings.autoHideFullscreen && !obsBg) {
        if (fullscreenFg) {
            if (!app.hiddenByFullscreen) {
                app.hiddenByFullscreen = true;
                ShowWindow(app.hwnd, SW_HIDE);
            }
        } else if (app.hiddenByFullscreen) {
            app.hiddenByFullscreen = false;
            ShowWindow(app.hwnd, SW_SHOWNOACTIVATE);
            app.lastTickTime = now;
            app.lastRenderTime = now;
            assertTopmost(app);
        }
    } else if (app.hiddenByFullscreen) {
        app.hiddenByFullscreen = false;
        if (app.isVisible) {
            ShowWindow(app.hwnd, SW_SHOWNOACTIVATE);
            app.lastTickTime = now;
            app.lastRenderTime = now;
            assertTopmost(app);
        }
    }

    const bool effectivelyVisible = app.isVisible && !app.hiddenByFullscreen;

    // Rendering stalled for >2s while visible (e.g. the layered window lost its surface): self-heal.
    if (effectivelyVisible && app.lastRenderTime > 0.0 && now - app.lastRenderTime > 2.0) {
        recoverAfterSystemEvent(app);
        return;
    }
    if (effectivelyVisible && !IsWindowVisible(app.hwnd)) ShowWindow(app.hwnd, SW_SHOWNOACTIVATE);

    if (effectivelyVisible && !obsBg && app.settings.alwaysOnTop) {
        if (app.pendingTopmostTime > 0.0 && now >= app.pendingTopmostTime) {
            app.pendingTopmostTime = 0.0;
            assertTopmost(app);
        } else if (fullscreenFg || hasVisibleWindowAbove(app.hwnd)) {
            assertTopmost(app);
        }
    }

    // Some fullscreen/DirectInput games reset or starve Raw Input when taking focus;
    // periodically re-register RIDEV_INPUTSINK while a fullscreen app is active and no raw keys arrive.
    if (fullscreenFg && app.input.secondsSinceRawKeyboard(now) > 3.0) {
        app.input.registerRawInput(app.hwnd);
        if (!app.input.kbHook_) app.input.installKeyboardHook();
    }

    // Detect admin-level apps (WPS Office/PPT, games, admin terminals) whose keystrokes Windows withholds (UIPI).
    app.foregroundBlocksKeys = false;
    if (!app.isElevated) {
        HWND fg = GetForegroundWindow();
        DWORD pid = 0;
        if (fg) GetWindowThreadProcessId(fg, &pid);
        if (pid && pid != GetCurrentProcessId() && isProcessElevatedOrProtected(pid)) {
            app.foregroundBlocksKeys = true;
            app.lastBlockedForegroundTime = now;
            if (app.blockedForegroundSince <= 0.0) app.blockedForegroundSince = now;
            if (!app.warnedElevatedForeground && (fullscreenFg || (now - app.blockedForegroundSince >= 1.0))) {
                app.warnedElevatedForeground = true;
                showTrayBalloon(app,
                    trW(L"KuroCat 听不到当前窗口的键盘",
                        L"KuroCat can't hear the keyboard in this window",
                        L"O KuroCat não consegue ouvir o teclado desta janela"),
                    trW(L"当前软件（如 WPS / 游戏）以管理员权限运行，Windows 会拦截它的键盘输入。右键小猫或托盘图标 →「以管理员身份重新启动」即可恢复。",
                        L"This app (e.g. WPS or a game) runs as administrator, so Windows withholds its keyboard input. Right-click the cat or the tray icon → \"Restart as Administrator\" to fix it.",
                        L"Este programa (ex.: WPS ou um jogo) roda como administrador e o Windows bloqueia o teclado dele. Clique com o botão direito no gato ou no ícone da bandeja → “Reiniciar como administrador” para resolver."));
            }
        } else {
            app.blockedForegroundSince = 0.0;
        }
    }
}

void quitApp(AppState& app) {
    app.input.uninstallKeyboardHook();
    // Destroying the overlay runs the normal WM_DESTROY cleanup (tray icon, hotkey...) and quits.
    if (app.hwnd && IsWindow(app.hwnd)) DestroyWindow(app.hwnd);
    else PostQuitMessage(0);
}

void setClickThrough(HWND hwnd, bool transparent) {
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    bool isTrans = (ex & WS_EX_TRANSPARENT) != 0;
    if (isTrans != transparent) {
        if (transparent) ex |= WS_EX_TRANSPARENT;
        else ex &= ~WS_EX_TRANSPARENT;
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
    }
}

void applyOBSMode(AppState& app) {
    if (!app.hwnd) return;
    LONG_PTR ex = GetWindowLongPtrW(app.hwnd, GWL_EXSTYLE);
    bool wantToolWindow = (app.settings.obsCaptureMode == 0);
    bool wantLayered = (app.settings.obsCaptureMode != 2);
    bool isToolWindow = (ex & WS_EX_TOOLWINDOW) != 0;
    bool isLayered = (ex & WS_EX_LAYERED) != 0;
    bool styleChanged = (wantToolWindow != isToolWindow || wantLayered != isLayered);
    if (styleChanged) {
        if (wantToolWindow) ex |= WS_EX_TOOLWINDOW;
        else ex &= ~WS_EX_TOOLWINDOW;
        if (wantLayered) ex |= WS_EX_LAYERED;
        else ex &= ~WS_EX_LAYERED;
        if (app.settings.obsCaptureMode == 2 && !app.settings.fullClickThrough) {
            ex &= ~WS_EX_TRANSPARENT;
        }
        bool wasVis = app.isVisible && !app.hiddenByFullscreen;
        if (wasVis) ShowWindow(app.hwnd, SW_HIDE);
        SetWindowLongPtrW(app.hwnd, GWL_EXSTYLE, ex);
        SetWindowPos(app.hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        if (wasVis) ShowWindow(app.hwnd, SW_SHOWNOACTIVATE);
    }
    bool hideDesk = (app.settings.obsCaptureMode > 0 && app.settings.obsHideDesktop);
    if (hideDesk) {
        // Keep 1px inside virtual screen edge so DWM continuously composites live frames for both WGC and BitBlt,
        // while staying tucked behind all windows (HWND_BOTTOM) and virtually invisible on the desktop.
        int offX = GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN) - 1;
        int offY = GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN) - 1;
        SetWindowPos(app.hwnd, HWND_BOTTOM, offX, offY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
    } else {
        applyWindowScaleAndPosition(app, false);
    }
    if (styleChanged) {
        RedrawWindow(app.hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME);
    }
}

// MARK: - Skins (built-in coats + imported .kurocat files)

const SkinSpec& activeSkin(const AppState& app) {
    if (!app.settings.customSkin.empty())
        if (const SkinSpec* s = app.skins.find(app.settings.customSkin)) return *s;
    return builtInSkin(app.settings.coatTheme);
}

// Name shown for a skin; built-in coats are localized in English / Portuguese.
std::wstring skinDisplayNameW(const AppState& app, const SkinSpec& s) {
    (void)app;
    if (s.builtIn && kurocat::uiLanguage() != kurocat::UiLanguage::Chinese
        && s.id.size() == 9 && s.id.compare(0, 8, "builtin:") == 0) {
        switch (s.id[8]) {
            case '0': return trW(L"", L"Classic Black Cat", L"Gatinho preto clássico");
            case '1': return trW(L"", L"Siamese Cat", L"Gato siamês");
            case '2': return trW(L"", L"Silver Tabby Shorthair", L"Pelo curto tigrado prateado");
            case '3': return trW(L"", L"Orange & White Cat", L"Gato laranja e branco");
            case '4': return trW(L"", L"Dazed Cat", L"Gato atordoado");
            case '5': return trW(L"", L"Tabby Cat", L"Gato tigrado");
            default: break;
        }
    }
    return utf8ToWide(s.name);
}

const KeyboardSkinSpec& builtInKeyboardSkin(KeyboardTheme theme) {
    static const KeyboardSkinSpec themes[3] = {
        KeyboardSkinSpec::builtInTheme(0),
        KeyboardSkinSpec::builtInTheme(1),
        KeyboardSkinSpec::builtInTheme(2)
    };
    int i = std::clamp((int)theme, 0, 2);
    return themes[i];
}

const KeyboardSkinSpec& activeKeyboardSkin(const AppState& app) {
    if (!app.settings.customKeyboard.empty())
        if (const KeyboardSkinSpec* k = app.kbLibrary.find(app.settings.customKeyboard)) return *k;
    return builtInKeyboardSkin(app.settings.keyboardTheme);
}

void allowDropsWhenElevated(HWND hwnd) {
    // An elevated KuroCat would otherwise never see files dragged from the (non-elevated) Explorer.
    ChangeWindowMessageFilterEx(hwnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
    ChangeWindowMessageFilterEx(hwnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr);
    ChangeWindowMessageFilterEx(hwnd, 0x0049 /* WM_COPYGLOBALDATA */, MSGFLT_ALLOW, nullptr);
}

void setDressingStatus(AppState& app, const std::wstring& text, int kind) {
    app.dressingStatusText = text;
    app.dressingStatusKind = kind;
    if (app.dressingRoom) InvalidateRect(app.dressingRoom, nullptr, FALSE);
}

void refreshSettingsWindow(AppState& app);

void refreshDressingRoom(AppState& app) {
    if (app.dressingRoom) InvalidateRect(app.dressingRoom, nullptr, FALSE);
    refreshSettingsWindow(app);
}

// Imports one file; on success switches to it. Errors are shown in Chinese.
bool importSkinFile(AppState& app, const std::wstring& path) {
    std::string id, error;
    std::wstring base = path.substr(path.find_last_of(L"\\/") == std::wstring::npos ? 0 : path.find_last_of(L"\\/") + 1);
    if (!app.skins.importFile(path, id, error)) {
        // With 换装间 open the red status line explains it in place; a dialog is only for drops onto the cat.
        if (app.dressingRoom) {
            setDressingStatus(app, trW(L"没能导入「", L"Couldn't import “", L"Não foi possível importar “") + base
                                   + trW(L"」：", L"”: ", L"”: ") + utf8ToWide(error), 2);
        } else {
            std::wstring msg = trW(L"「", L"“", L"“") + base + trW(L"」：", L"”: ", L"”: ") + utf8ToWide(error);
            MessageBoxW(app.menuOwnerHwnd, msg.c_str(),
                        trW(L"无法导入这个皮肤", L"Failed to Import Skin", L"Não foi possível importar a skin"),
                        MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
        }
        return false;
    }
    app.settings.overrideSkinIris = false;
    app.settings.customSkin = id;
    app.settings.saveToRegistry();
    const SkinSpec* s = app.skins.find(id);
    std::wstring msg = trW(L"✓ 已换上「", L"✓ Now wearing “", L"✓ Agora vestindo “")
        + (s ? utf8ToWide(s->name) : std::wstring(trW(L"新皮肤", L"new skin", L"nova skin"))) + trW(L"」", L"”", L"”");
    if (s && !s->author.empty()) msg += trW(L"（作者：", L" (by ", L" (por ") + utf8ToWide(s->author) + trW(L"）", L")", L")");
    if (s) {
        int dup = 0;
        for (const auto& item : app.skins.skins) if (item.name == s->name) ++dup;
        if (dup > 1) msg += trW(L" · 存在同名皮肤，已附加短编号区分",
                                L" · A skin with this name exists; a short ID was added",
                                L" · Já existe uma skin com esse nome; foi adicionado um código curto");
    }
    setDressingStatus(app, msg, 1);
    if (!app.dressingRoom) showTrayBalloon(app, trW(L"KuroCat 换装间", L"KuroCat Dressing Room", L"KuroCat – Provador"), msg.c_str());
    return true;
}

bool importKeyboardSkinFile(AppState& app, const std::wstring& path) {
    std::string id, error;
    size_t slash = path.find_last_of(L"\\/");
    std::wstring base = path.substr(slash == std::wstring::npos ? 0 : slash + 1);
    if (!app.kbLibrary.importFile(path, id, error)) {
        if (app.dressingRoom) {
            setDressingStatus(app, trW(L"没能导入键盘皮肤「", L"Couldn't import keyboard skin “", L"Não foi possível importar a skin de teclado “") + base
                                   + trW(L"」：", L"”: ", L"”: ") + utf8ToWide(error), 2);
        } else {
            std::wstring msg = trW(L"「", L"“", L"“") + base + trW(L"」：", L"”: ", L"”: ") + utf8ToWide(error);
            MessageBoxW(app.menuOwnerHwnd, msg.c_str(),
                        trW(L"无法导入这个键盘皮肤", L"Failed to Import Keyboard Skin", L"Não foi possível importar a skin de teclado"),
                        MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
        }
        return false;
    }
    app.settings.customKeyboard = id;
    app.settings.saveToRegistry();
    const KeyboardSkinSpec* kb = app.kbLibrary.find(id);
    std::wstring msg = trW(L"✓ 已换上键盘皮肤「", L"✓ Keyboard skin applied: “", L"✓ Skin de teclado aplicada: “")
        + (kb ? utf8ToWide(kb->name) : std::wstring(trW(L"新键盘", L"new keyboard", L"novo teclado"))) + trW(L"」", L"”", L"”");
    if (kb && !kb->author.empty()) msg += trW(L"（作者：", L" (by ", L" (por ") + utf8ToWide(kb->author) + trW(L"）", L")", L")");
    setDressingStatus(app, msg, 1);
    if (!app.dressingRoom) showTrayBalloon(app, trW(L"KuroCat 键盘皮肤", L"KuroCat Keyboard Skin", L"KuroCat – Skin de teclado"), msg.c_str());
    return true;
}

bool hasExtensionIgnoreCase(const std::wstring& path, const wchar_t* targetExt) {
    size_t slash = path.find_last_of(L"\\/");
    size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos || (slash != std::wstring::npos && dot < slash)) return false;
    return _wcsicmp(path.c_str() + dot + 1, targetExt) == 0;
}

void importDroppedFiles(AppState& app, HDROP drop) {
    UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
    std::vector<std::wstring> files;
    for (UINT i = 0; i < count && i < 16; ++i) {
        UINT len = DragQueryFileW(drop, i, nullptr, 0);
        std::wstring f(len + 1, L'\0');
        DragQueryFileW(drop, i, &f[0], len + 1);
        f.resize(len);
        files.push_back(f);
    }
    DragFinish(drop);
    for (const auto& f : files) {
        if (KeyboardSkinLibrary::looksLikeKeyboardFile(f)) {
            importKeyboardSkinFile(app, f);
        } else if (hasExtensionIgnoreCase(f, L"kurocat")) {
            importSkinFile(app, f);
        }
    }
}

void chooseSkinFile(AppState& app) {
    // comdlg32 is loaded on demand so the build keeps its existing link libraries.
    typedef BOOL (WINAPI *GetOpenFileNameFn)(LPOPENFILENAMEW);
    HMODULE dlg = LoadLibraryW(L"comdlg32.dll");
    auto getOpen = dlg ? (GetOpenFileNameFn)(void*)GetProcAddress(dlg, "GetOpenFileNameW") : nullptr;
    if (!getOpen) {
        if (dlg) FreeLibrary(dlg);
        MessageBoxW(app.dressingRoom,
                    trW(L"无法打开文件选择框，请直接把 .kurocat 文件拖进来。",
                        L"Unable to open the file dialog. Please drag the .kurocat file in here.",
                        L"Não foi possível abrir a janela de arquivos. Arraste o arquivo .kurocat para cá."),
                    trW(L"KuroCat 换装间", L"KuroCat Dressing Room", L"KuroCat – Provador"), MB_OK | MB_ICONWARNING);
        return;
    }
    wchar_t file[MAX_PATH * 4] = {0};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = app.dressingRoom;
    ofn.lpstrFilter = trW(L"KuroCat 皮肤 (*.kurocat)\0*.kurocat\0所有文件 (*.*)\0*.*\0",
                          L"KuroCat Skin (*.kurocat)\0*.kurocat\0All Files (*.*)\0*.*\0",
                          L"Skin do KuroCat (*.kurocat)\0*.kurocat\0Todos os arquivos (*.*)\0*.*\0");
    ofn.lpstrFile = file;
    ofn.nMaxFile = (DWORD)(sizeof(file) / sizeof(file[0]));
    ofn.lpstrTitle = trW(L"选择 .kurocat 皮肤文件", L"Choose a .kurocat Skin File", L"Escolher arquivo de skin .kurocat");
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
    BOOL ok = getOpen(&ofn);
    FreeLibrary(dlg);
    if (ok) {
        if (KeyboardSkinLibrary::looksLikeKeyboardFile(file)) importKeyboardSkinFile(app, file);
        else importSkinFile(app, file);
    }
}

void chooseKeyboardSkinFile(AppState& app) {
    typedef BOOL (WINAPI *GetOpenFileNameFn)(LPOPENFILENAMEW);
    HMODULE dlg = LoadLibraryW(L"comdlg32.dll");
    auto getOpen = dlg ? (GetOpenFileNameFn)(void*)GetProcAddress(dlg, "GetOpenFileNameW") : nullptr;
    if (!getOpen) {
        if (dlg) FreeLibrary(dlg);
        MessageBoxW(app.menuOwnerHwnd,
                    trW(L"无法打开文件选择框，请直接把 .kurokb / .json 文件拖到小猫身上。",
                        L"Unable to open file dialog. Please drag a .kurokb or .json file onto the cat.",
                        L"Não foi possível abrir a janela de arquivos. Arraste um arquivo .kurokb ou .json até o gatinho."),
                    L"KuroCat", MB_OK | MB_ICONWARNING | MB_TOPMOST);
        return;
    }
    wchar_t file[MAX_PATH * 4] = {0};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = app.menuOwnerHwnd;
    ofn.lpstrFilter = trW(L"KuroCat 键盘皮肤 (*.kurokb;*.json)\0*.kurokb;*.json\0所有文件 (*.*)\0*.*\0",
                          L"KuroCat Keyboard Skin (*.kurokb;*.json)\0*.kurokb;*.json\0All Files (*.*)\0*.*\0",
                          L"Skin de teclado do KuroCat (*.kurokb;*.json)\0*.kurokb;*.json\0Todos os arquivos (*.*)\0*.*\0");
    ofn.lpstrFile = file;
    ofn.nMaxFile = (DWORD)(sizeof(file) / sizeof(file[0]));
    ofn.lpstrTitle = trW(L"选择键盘皮肤文件 (.kurokb / .json)",
                         L"Choose Keyboard Skin (.kurokb / .json)",
                         L"Escolher skin de teclado (.kurokb / .json)");
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
    BOOL ok = getOpen(&ofn);
    FreeLibrary(dlg);
    if (ok) importKeyboardSkinFile(app, file);
}

// MARK: 「换装间」 window — GDI+ painted: live preview of the worn skin, dashed drop zone, pill button.
// Layout in 96-dpi design units, scaled by AppState::dressingScale.
namespace dressing {
constexpr float W = 444, H = 472;
const Gdiplus::RectF previewRect(22, 22, 150, 134);
const Gdiplus::RectF dropRect(22, 172, 400, 150);
const Gdiplus::RectF buttonRect(147, 338, 150, 40);

Gdiplus::FontFamily* family() {
    static Gdiplus::FontFamily* fam = nullptr;
    if (!fam) {
        for (const wchar_t* name : {L"Microsoft YaHei UI", L"Microsoft YaHei", L"Segoe UI"}) {
            auto* f = new Gdiplus::FontFamily(name);
            if (f->GetLastStatus() == Gdiplus::Ok) { fam = f; break; }
            delete f;
        }
        if (!fam) fam = Gdiplus::FontFamily::GenericSansSerif()->Clone();
    }
    return fam;
}
void roundRect(Gdiplus::GraphicsPath& p, const Gdiplus::RectF& r, float rad) {
    float d = rad * 2;
    p.AddArc(r.X, r.Y, d, d, 180, 90);
    p.AddArc(r.X + r.Width - d, r.Y, d, d, 270, 90);
    p.AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0, 90);
    p.AddArc(r.X, r.Y + r.Height - d, d, d, 90, 90);
    p.CloseFigure();
}
void text(Gdiplus::Graphics& g, const std::wstring& s, float size, int style, Gdiplus::Color c, const Gdiplus::RectF& box,
          Gdiplus::StringAlignment align = Gdiplus::StringAlignmentNear, bool ellipsis = false) {
    Gdiplus::Font font(family(), size, style, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush brush(c);
    Gdiplus::StringFormat fmt;
    fmt.SetAlignment(align);
    if (ellipsis) { fmt.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter); fmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap); }
    g.DrawString(s.c_str(), (INT)s.size(), &font, box, &fmt, &brush);
}
void paw(Gdiplus::Graphics& g, float cx, float cy, Gdiplus::Color c) {
    Gdiplus::SolidBrush b(c);
    g.FillEllipse(&b, cx - 9.5f, cy + 6 - 7.8f, 19.0f, 15.6f);
    const float toes[4][2] = {{-10.5f, -5.5f}, {-3.8f, -11.5f}, {3.8f, -11.5f}, {10.5f, -5.5f}};
    for (const auto& t : toes) g.FillEllipse(&b, cx + t[0] - 3.7f, cy + t[1] - 4.4f, 7.4f, 8.8f);
}

void paint(AppState& app, HWND hwnd, HDC hdc) {
    using namespace Gdiplus;
    RECT rc; GetClientRect(hwnd, &rc);
    int cw = std::max(1, (int)rc.right), ch = std::max(1, (int)rc.bottom);
    Bitmap back(cw, ch, PixelFormat32bppPARGB);
    Graphics g(&back);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.Clear(Color(255, 251, 247, 240));
    g.ScaleTransform(app.dressingScale, app.dressingScale);

    // Live preview of the skin being worn
    GraphicsPath card; roundRect(card, previewRect, 18);
    SolidBrush cardBrush(Color(255, 243, 233, 220));
    g.FillPath(&cardBrush, &card);
    {
        GraphicsState st = g.Save();
        g.SetClip(&card, CombineModeIntersect);
        g.TranslateTransform(previewRect.X, previewRect.Y);
        g.ScaleTransform(previewRect.Width / 290.0f, previewRect.Height / 260.0f);
        g.TranslateTransform(-40.0f, -36.0f);
        static CatPhysics pose;   // a calm sitting pose
        pose.time = 0.7f;
        CatSettings settings = app.settings;
        settings.showKeyLegends = false;
        settings.isEditMode = false;
        settings.flipHorizontal = false;
        settings.rotationDeg = 0.0f;
        CatRenderer::draw(g, pose, settings, activeSkin(app), activeKeyboardSkin(app));
        g.Restore(st);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    }

    const SkinSpec& skin = activeSkin(app);
    const Color ink(255, 59, 47, 42), brown(255, 90, 74, 64), muted(255, 168, 153, 141), orange(255, 232, 150, 78);
    text(g, trW(L"换装间", L"Dressing Room", L"Provador"), 21, FontStyleBold, ink, RectF(190, 22, 240, 30));
    text(g, trW(L"正在穿", L"Wearing", L"Vestindo agora"), 11.5f, FontStyleRegular, muted, RectF(191, 58, 230, 18));
    text(g, skinDisplayNameW(app, skin), 15, FontStyleBold, brown, RectF(190, 74, 236, 22), StringAlignmentNear, true);
    if (!skin.author.empty()) text(g, trW(L"作者 ", L"By ", L"Por ") + utf8ToWide(skin.author), 11.5f, FontStyleRegular, muted, RectF(191, 97, 232, 18), StringAlignmentNear, true);
    text(g, trW(L"导入皮肤工坊做好的 .kurocat 文件，\n小猫会立刻换上新毛色。",
                L"Import a .kurocat from the Skin Workshop\nand the cat wears it instantly.",
                L"Importe uma skin .kurocat da oficina\ne o gatinho muda de pelagem na hora."),
         11.5f, FontStyleRegular, muted, RectF(191, 120, 236, 40));

    // Drop zone
    GraphicsPath zone; roundRect(zone, dropRect, 16);
    SolidBrush zoneBrush(Color(255, 255, 253, 249));
    g.FillPath(&zoneBrush, &zone);
    Pen dash(Color(255, 227, 195, 160), 2.0f);
    REAL pattern[2] = {3.5f, 2.5f};
    dash.SetDashPattern(pattern, 2);
    g.DrawPath(&dash, &zone);
    float cx = dropRect.X + dropRect.Width / 2, cy = dropRect.Y + dropRect.Height / 2;
    paw(g, cx, cy - 26, Color(230, 232, 150, 78));
    text(g, trW(L"把 .kurocat 皮肤拖到这里", L"Drag a .kurocat skin here", L"Arraste uma skin .kurocat para cá"), 15, FontStyleBold, brown, RectF(dropRect.X, cy + 2, dropRect.Width, 24), StringAlignmentCenter);
    text(g, trW(L"也可以直接拖到桌面上的小猫身上 · 点这里选择文件",
                L"Or drop it on the desktop cat · Click here to choose a file",
                L"Ou solte no gatinho da área de trabalho · clique para escolher"), 11.5f, FontStyleRegular, Color(255, 160, 143, 130),
         RectF(dropRect.X, cy + 28, dropRect.Width, 20), StringAlignmentCenter);

    // Status + footer
    Color statusColor = app.dressingStatusKind == 1 ? Color(255, 62, 142, 90)
                      : app.dressingStatusKind == 2 ? Color(255, 194, 73, 59) : Color(255, 138, 123, 112);
    text(g, app.dressingStatusText, 12.5f, app.dressingStatusKind ? FontStyleBold : FontStyleRegular, statusColor,
         RectF(22, 390, 400, 42), StringAlignmentCenter);
    text(g, (std::wstring(trW(L"皮肤独立保存在 ", L"Skins are saved in ", L"Skins salvas em ")) + SkinLibrary::dir()
             + trW(L"（软件更新会自动保留）  ·  右键小猫 → 猫咪皮肤 可切换或备份",
                   L" (kept across updates)  ·  Right-click the cat → Cat Skin to switch or back up",
                   L" (mantidas ao atualizar)  ·  Botão direito no gato → Skin do gato para trocar")).c_str(),
         10.0f, FontStyleRegular, Color(255, 179, 166, 155), RectF(18, 434, 408, 34), StringAlignmentCenter);
    (void)orange;

    Graphics screen(hdc);
    screen.DrawImage(&back, 0, 0, cw, ch);
}

void paintButton(AppState& app, const DRAWITEMSTRUCT* di) {
    using namespace Gdiplus;
    int w = di->rcItem.right - di->rcItem.left, h = di->rcItem.bottom - di->rcItem.top;
    Bitmap back(std::max(1, w), std::max(1, h), PixelFormat32bppPARGB);
    Graphics g(&back);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.Clear(Color(255, 251, 247, 240));
    float k = app.dressingScale;
    bool pressed = (di->itemState & ODS_SELECTED) != 0;
    GraphicsPath pill; roundRect(pill, RectF(1, 1, w - 2.0f, h - 2.0f), (h - 2.0f) / 2);
    SolidBrush fill(pressed ? Color(255, 215, 132, 62) : Color(255, 232, 150, 78));
    g.FillPath(&fill, &pill);
    Font font(family(), 14.5f * k, FontStyleBold, UnitPixel);
    SolidBrush white(Color(255, 255, 255, 255));
    StringFormat fmt; fmt.SetAlignment(StringAlignmentCenter); fmt.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(trW(L"选择文件…", L"Choose File…", L"Escolher arquivo…"), -1, &font, RectF(0, 0, (REAL)w, (REAL)h), &fmt, &white);
    if (di->itemState & ODS_FOCUS) {
        Pen ring(Color(120, 232, 150, 78), 2.0f);
        g.DrawPath(&ring, &pill);
    }
    Graphics screen(di->hDC);
    screen.DrawImage(&back, di->rcItem.left, di->rcItem.top, w, h);
}

bool inDrop(AppState& app, LPARAM lParam) {
    float x = (short)LOWORD(lParam) / app.dressingScale, y = (short)HIWORD(lParam) / app.dressingScale;
    return x >= dropRect.X && x <= dropRect.X + dropRect.Width && y >= dropRect.Y && y <= dropRect.Y + dropRect.Height;
}
} // namespace dressing

LRESULT CALLBACK dressingRoomProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* app = g_app;
    switch (msg) {
        case WM_DROPFILES:
            if (app) importDroppedFiles(*app, (HDROP)wParam);
            else DragFinish((HDROP)wParam);
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_DRESSING_CHOOSE && app) chooseSkinFile(*app);
            return 0;
        case WM_DRAWITEM:
            if (app && wParam == IDC_DRESSING_CHOOSE) { dressing::paintButton(*app, (const DRAWITEMSTRUCT*)lParam); return TRUE; }
            break;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (app) dressing::paint(*app, hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_SETCURSOR:
            if (app && LOWORD(lParam) == HTCLIENT) {
                POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
                if (dressing::inDrop(*app, MAKELPARAM(pt.x, pt.y))) { SetCursor(LoadCursorW(nullptr, IDC_HAND)); return TRUE; }
            }
            break;
        case WM_LBUTTONUP:
            if (app && dressing::inDrop(*app, lParam)) chooseSkinFile(*app);
            return 0;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (app && app->dressingRoom == hwnd) app->dressingRoom = nullptr;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void showDressingRoom(AppState& app) {
    if (app.dressingRoom) { refreshDressingRoom(app); ShowWindow(app.dressingRoom, SW_SHOWNORMAL); SetForegroundWindow(app.dressingRoom); return; }
    HINSTANCE inst = GetModuleHandleW(nullptr);
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = dressingRoomProc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;   // fully painted in WM_PAINT
        wc.lpszClassName = DRESSING_ROOM_CLASS;
        registered = RegisterClassExW(&wc) != 0;
    }
    HDC screen = GetDC(nullptr);
    float dpiScale = GetDeviceCaps(screen, LOGPIXELSY) / 96.0f;
    ReleaseDC(nullptr, screen);
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, exStyle = WS_EX_TOPMOST | WS_EX_ACCEPTFILES;
    RECT frameRc = { 0, 0, 0, 0 };
    AdjustWindowRectEx(&frameRc, style, FALSE, exStyle);
    int frameW = frameRc.right - frameRc.left, frameH = frameRc.bottom - frameRc.top;
    RECT work = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int maxClientW = std::max(280, (int)(work.right - work.left) - frameW - 24);
    int maxClientH = std::max(240, (int)(work.bottom - work.top) - frameH - 24);
    float k = std::min({ dpiScale, (float)maxClientW / dressing::W, (float)maxClientH / dressing::H });
    if (k < 0.65f) k = 0.65f;
    app.dressingScale = k;
    auto px = [&](float v) { return (int)std::lround(v * k); };
    RECT rc = {0, 0, px(dressing::W), px(dressing::H)};
    AdjustWindowRectEx(&rc, style, FALSE, exStyle);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    int posX = std::max((int)work.left, (int)(work.left + work.right - w) / 2);
    int posY = std::max((int)work.top, (int)(work.top + work.bottom - h) / 2);
    HWND win = CreateWindowExW(exStyle, DRESSING_ROOM_CLASS, trW(L"换装间", L"Dressing Room", L"Provador"), style,
        posX, posY, w, h, nullptr, nullptr, inst, nullptr);
    if (!win) return;
    app.dressingRoom = win;
    allowDropsWhenElevated(win);
    DragAcceptFiles(win, TRUE);
    const Gdiplus::RectF& b = dressing::buttonRect;
    CreateWindowExW(0, L"BUTTON", trW(L"选择文件…", L"Choose File…", L"Escolher arquivo…"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                    px(b.X), px(b.Y), px(b.Width), px(b.Height), win, (HMENU)(INT_PTR)IDC_DRESSING_CHOOSE, inst, nullptr);
    setDressingStatus(app, trW(L"支持拖入多个文件；坏掉的文件会告诉你哪里不对。",
                               L"You can drop several files at once; broken files will tell you what's wrong.",
                               L"Dá para soltar vários arquivos; se algum estiver com problema, você verá o motivo."), 0);
    ShowWindow(win, SW_SHOWNORMAL);
    SetForegroundWindow(win);
}

void refreshSettingsWindow(AppState& app);

struct RenameDialogState {
    std::wstring value;
    bool accepted = false;
    bool done = false;
    HWND editHwnd = nullptr;
};

static LRESULT CALLBACK RenameSkinDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* st = (RenameDialogState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg) {
        case WM_NCCREATE: {
            auto* cs = (CREATESTRUCTW*)lParam;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return TRUE;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDOK && st) {
                wchar_t buf[128] = {};
                GetWindowTextW(st->editHwnd, buf, 127);
                st->value = buf;
                st->accepted = true;
                st->done = true;
                DestroyWindow(hwnd);
                return 0;
            }
            if (id == IDCANCEL && st) {
                st->accepted = false;
                st->done = true;
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            if (st) { st->accepted = false; st->done = true; }
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool promptRenameSkin(AppState& app, const std::string& id) {
    const SkinSpec* s = app.skins.find(id);
    if (!s) return false;
    static bool reg = false;
    const wchar_t* clsName = L"KuroCatRenameSkinDlg";
    HINSTANCE inst = GetModuleHandleW(nullptr);
    if (!reg) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = RenameSkinDlgProc;
        wc.hInstance = inst;
        wc.lpszClassName = clsName;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassW(&wc);
        reg = true;
    }
    auto tr = [&](const wchar_t* zh, const wchar_t* eng, const wchar_t* pt) -> const wchar_t* { return app.settings.tr(zh, eng, pt); };
    RenameDialogState st;
    st.value = utf8ToWide(s->name);

    int w = 360, h = 150;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    HWND dlg = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_DLGMODALFRAME,
        clsName,
        tr(L"重命名皮肤", L"Rename Skin", L"Renomear skin"),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        sx, sy, w, h,
        app.settingsWnd ? app.settingsWnd : app.menuOwnerHwnd,
        nullptr, inst, &st
    );
    if (!dlg) return false;

    HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND lbl = CreateWindowExW(0, L"STATIC",
        tr(L"请输入新的皮肤名称（最多 40 字）：", L"Enter a new skin name (max 40 chars):", L"Novo nome da skin (máx. 40 caracteres):"),
        WS_CHILD | WS_VISIBLE, 18, 16, 316, 20, dlg, nullptr, inst, nullptr);
    st.editHwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", st.value.c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 18, 42, 316, 24, dlg, (HMENU)1001, inst, nullptr);
    HWND okBtn = CreateWindowExW(0, L"BUTTON", tr(L"保存", L"Save", L"Salvar"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 174, 80, 76, 26, dlg, (HMENU)IDOK, inst, nullptr);
    HWND cancelBtn = CreateWindowExW(0, L"BUTTON", tr(L"取消", L"Cancel", L"Cancelar"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 258, 80, 76, 26, dlg, (HMENU)IDCANCEL, inst, nullptr);
    for (HWND c : {lbl, st.editHwnd, okBtn, cancelBtn}) SendMessageW(c, WM_SETFONT, (WPARAM)font, TRUE);
    SendMessageW(st.editHwnd, EM_SETLIMITTEXT, 40, 0);
    SendMessageW(st.editHwnd, EM_SETSEL, 0, -1);
    SetForegroundWindow(dlg);
    SetFocus(st.editHwnd);

    MSG m;
    while (!st.done && GetMessageW(&m, nullptr, 0, 0) > 0) {
        if (m.message == WM_KEYDOWN && m.wParam == VK_RETURN) {
            PostMessageW(dlg, WM_COMMAND, IDOK, 0);
            continue;
        }
        if (m.message == WM_KEYDOWN && m.wParam == VK_ESCAPE) {
            PostMessageW(dlg, WM_COMMAND, IDCANCEL, 0);
            continue;
        }
        if (!IsDialogMessageW(dlg, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }
    if (!st.accepted) return false;

    std::string newNameUtf8 = wideToUtf8(st.value);
    std::string newId, err;
    if (!app.skins.rename(id, newNameUtf8, newId, err)) {
        MessageBoxW(app.menuOwnerHwnd, utf8ToWide(err).c_str(),
                    tr(L"重命名失败", L"Rename Failed", L"Falha ao renomear"), MB_OK | MB_ICONWARNING | MB_TOPMOST);
        return false;
    }
    if (app.settings.customSkin == id) {
        app.settings.customSkin = newId;
        app.settings.saveToRegistry();
    }
    refreshDressingRoom(app);
    refreshSettingsWindow(app);
    return true;
}

void confirmDeleteSkin(AppState& app, const std::string& id) {
    const SkinSpec* s = app.skins.find(id);
    if (!s) return;
    std::wstring q = trW(L"删除皮肤「", L"Delete the skin “", L"Excluir a skin “") + utf8ToWide(s->name)
        + trW(L"」？\n\n会从皮肤文件夹里删除这个文件，删除后需要重新导入。",
              L"”?\n\nThe file will be removed from the skins folder; you'll have to import it again to get it back.",
              L"”?\n\nO arquivo será apagado da pasta de skins; para usá-la de novo, será preciso importá-la outra vez.");
    if (MessageBoxW(app.menuOwnerHwnd, q.c_str(), L"KuroCat", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST | MB_SETFOREGROUND | MB_DEFBUTTON2) != IDYES) return;
    std::string removing = id;
    if (!app.skins.remove(removing)) {
        MessageBoxW(app.menuOwnerHwnd, (trW(L"无法删除 ", L"Couldn't delete ", L"Não foi possível excluir ") + SkinLibrary::dir() + L"\\" + utf8ToWide(removing) + L".kurocat").c_str(),
                    trW(L"删除失败", L"Delete Failed", L"Falha ao excluir"), MB_OK | MB_ICONWARNING | MB_TOPMOST);
        return;
    }
    if (app.settings.customSkin == removing) { app.settings.customSkin.clear(); app.settings.saveToRegistry(); }
    refreshDressingRoom(app);
    refreshSettingsWindow(app);
}

struct BatchDeleteSkinDlgState {
    HWND listHwnd = nullptr;
    HWND countLbl = nullptr;
    HWND delBtn = nullptr;
    std::vector<std::string> ids;
    std::vector<std::string> selectedIds;
    bool accepted = false;
    bool done = false;
};

static void updateBatchDeleteUiCount(BatchDeleteSkinDlgState& st) {
    if (!st.listHwnd) return;
    int selCount = (int)SendMessageW(st.listHwnd, LB_GETSELCOUNT, 0, 0);
    if (selCount < 0) selCount = 0;
    if (st.countLbl) {
        wchar_t buf[64] = {};
        swprintf_s(buf, trW(L"\u5DF2\u9009\uFF1A%d / %zu \u6B3E", L"Selected: %d / %zu", L"Selecionadas: %d / %zu"), selCount, st.ids.size());
        SetWindowTextW(st.countLbl, buf);
    }
    if (st.delBtn) {
        wchar_t buf[64] = {};
        swprintf_s(buf, trW(L"\u5220\u9664\u6240\u9009 (%d)", L"Delete (%d)", L"Excluir (%d)"), selCount);
        SetWindowTextW(st.delBtn, buf);
        EnableWindow(st.delBtn, selCount > 0 ? TRUE : FALSE);
    }
}

static LRESULT CALLBACK BatchDeleteSkinDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* st = (BatchDeleteSkinDlgState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg) {
        case WM_NCCREATE: {
            auto* cs = (CREATESTRUCTW*)lParam;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return TRUE;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);
            if (!st) break;
            if (id == 1001 && code == LBN_SELCHANGE) {
                updateBatchDeleteUiCount(*st);
                return 0;
            }
            if (id == 1002) { // Select All
                SendMessageW(st->listHwnd, LB_SETSEL, TRUE, -1);
                updateBatchDeleteUiCount(*st);
                return 0;
            }
            if (id == 1003) { // Clear
                SendMessageW(st->listHwnd, LB_SETSEL, FALSE, -1);
                updateBatchDeleteUiCount(*st);
                return 0;
            }
            if (id == IDOK) {
                int selCount = (int)SendMessageW(st->listHwnd, LB_GETSELCOUNT, 0, 0);
                if (selCount <= 0) return 0;
                std::vector<int> indices((size_t)selCount);
                SendMessageW(st->listHwnd, LB_GETSELITEMS, (WPARAM)selCount, (LPARAM)indices.data());
                st->selectedIds.clear();
                for (int idx : indices) {
                    if (idx >= 0 && (size_t)idx < st->ids.size()) st->selectedIds.push_back(st->ids[(size_t)idx]);
                }
                st->accepted = !st->selectedIds.empty();
                st->done = true;
                DestroyWindow(hwnd);
                return 0;
            }
            if (id == IDCANCEL) {
                st->accepted = false;
                st->done = true;
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            if (st) { st->accepted = false; st->done = true; }
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void confirmBatchDeleteSkins(AppState& app) {
    if (app.skins.skins.empty()) {
        MessageBoxW(app.settingsWnd ? app.settingsWnd : app.menuOwnerHwnd,
                    trW(L"\u6682\u65E0\u5DF2\u5BFC\u5165\u7684\u81EA\u5B9A\u4E49\u76AE\u80A4\u3002", L"No imported custom skins yet.", L"Nenhuma skin importada ainda."),
                    L"KuroCat", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        return;
    }

    static bool reg = false;
    const wchar_t* clsName = L"KuroCatBatchDeleteSkinDlg";
    HINSTANCE inst = GetModuleHandleW(nullptr);
    if (!reg) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = BatchDeleteSkinDlgProc;
        wc.hInstance = inst;
        wc.lpszClassName = clsName;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassW(&wc);
        reg = true;
    }

    BatchDeleteSkinDlgState st;
    int w = 420, h = 420;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    HWND dlg = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_DLGMODALFRAME,
        clsName,
        trW(L"\u6279\u91CF\u5220\u9664\u5DF2\u5BFC\u5165\u76AE\u80A4", L"Batch Delete Imported Skins", L"Excluir várias skins importadas"),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        sx, sy, w, h,
        app.settingsWnd ? app.settingsWnd : app.menuOwnerHwnd,
        nullptr, inst, &st
    );
    if (!dlg) return;

    HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND tipLbl = CreateWindowExW(0, L"STATIC",
        trW(L"\u70B9\u51FB\u5217\u8868\u591A\u9009\u8981\u5220\u9664\u7684\u76AE\u80A4\uFF08\u5185\u7F6E\u732B\u54AA\u4E0D\u53D7\u5F71\u54CD\uFF09\uFF1A", L"Click items to toggle selection (built-in cats stay safe):", L"Clique para marcar as skins (as nativas não são afetadas):"),
        WS_CHILD | WS_VISIBLE, 16, 14, 376, 20, dlg, nullptr, inst, nullptr);
    HWND selAllBtn = CreateWindowExW(0, L"BUTTON", trW(L"\u5168\u9009", L"Select All", L"Todas"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 16, 38, 72, 24, dlg, (HMENU)1002, inst, nullptr);
    HWND selNoneBtn = CreateWindowExW(0, L"BUTTON", trW(L"\u6E05\u7A7A", L"Clear", L"Nenhuma"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 94, 38, 72, 24, dlg, (HMENU)1003, inst, nullptr);
    st.countLbl = CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_RIGHT, 220, 42, 172, 18, dlg, nullptr, inst, nullptr);

    st.listHwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | LBS_MULTIPLESEL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        16, 68, 376, 260, dlg, (HMENU)1001, inst, nullptr);

    for (const auto& sk : app.skins.skins) {
        st.ids.push_back(sk.id);
        std::string line = app.skins.displayName(sk);
        if (sk.id == app.settings.customSkin) line += kurocat::uiText(" [正在穿]", " [Active]", " [em uso]");
        if (!sk.author.empty()) line += " - " + sk.author;
        SendMessageW(st.listHwnd, LB_ADDSTRING, 0, (LPARAM)utf8ToWide(line).c_str());
    }

    HWND cancelBtn = CreateWindowExW(0, L"BUTTON", trW(L"\u53D6\u6D88", L"Cancel", L"Cancelar"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 16, 340, 88, 28, dlg, (HMENU)IDCANCEL, inst, nullptr);
    st.delBtn = CreateWindowExW(0, L"BUTTON", trW(L"\u5220\u9664\u6240\u9009 (0)", L"Delete (0)", L"Excluir (0)"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 252, 340, 140, 28, dlg, (HMENU)IDOK, inst, nullptr);

    for (HWND c : {tipLbl, selAllBtn, selNoneBtn, st.countLbl, st.listHwnd, cancelBtn, st.delBtn}) {
        SendMessageW(c, WM_SETFONT, (WPARAM)font, TRUE);
    }
    updateBatchDeleteUiCount(st);
    SetForegroundWindow(dlg);
    SetFocus(st.listHwnd);

    MSG m;
    while (!st.done && GetMessageW(&m, nullptr, 0, 0) > 0) {
        if (m.message == WM_KEYDOWN && m.wParam == VK_ESCAPE) {
            PostMessageW(dlg, WM_COMMAND, IDCANCEL, 0);
            continue;
        }
        if (!IsDialogMessageW(dlg, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }
    if (!st.accepted || st.selectedIds.empty()) return;

    const std::wstring selN = std::to_wstring(st.selectedIds.size());
    std::wstring msgText = trW(L"\u786E\u8BA4\u6279\u91CF\u5220\u9664\u8FD9 ", L"Delete ", L"Excluir ") + selN
        + trW(L" \u6B3E\u76AE\u80A4\uFF1F\n\n\u4F1A\u4ECE\u76AE\u80A4\u6587\u4EF6\u5939\u91CC\u5220\u9664\u5BF9\u5E94\u6587\u4EF6\uFF0C\u5220\u9664\u540E\u9700\u8981\u91CD\u65B0\u5BFC\u5165\u3002",
              L" selected .kurocat skin file(s)?",
              L" arquivo(s) de skin .kurocat selecionado(s)?\n\nEles serão apagados da pasta de skins; para usá-los de novo será preciso importar outra vez.");
    if (MessageBoxW(app.settingsWnd ? app.settingsWnd : app.menuOwnerHwnd, msgText.c_str(), L"KuroCat",
                    MB_YESNO | MB_ICONQUESTION | MB_TOPMOST | MB_SETFOREGROUND | MB_DEFBUTTON2) != IDYES) {
        return;
    }

    std::vector<std::string> failed;
    std::vector<std::string> removed = app.skins.removeMany(st.selectedIds, &failed);
    if (std::find(removed.begin(), removed.end(), app.settings.customSkin) != removed.end()) {
        app.settings.customSkin.clear();
        app.settings.saveToRegistry();
    }
    if (!failed.empty()) {
        std::wstring warn = trW(L"\u5DF2\u5220\u9664 ", L"Deleted ", L"Excluídas: ") + std::to_wstring(removed.size())
            + trW(L" \u6B3E\uFF0C\u6709 ", L", failed ", L"; com falha: ") + std::to_wstring(failed.size())
            + trW(L" \u6B3E\u65E0\u6CD5\u5220\u9664\u3002", L".", L".");
        MessageBoxW(app.settingsWnd ? app.settingsWnd : app.menuOwnerHwnd, warn.c_str(), L"KuroCat", MB_OK | MB_ICONWARNING | MB_TOPMOST);
    }
    refreshDressingRoom(app);
    refreshSettingsWindow(app);
}

void confirmDeleteKeyboardSkin(AppState& app, const std::string& id) {
    const KeyboardSkinSpec* kb = app.kbLibrary.find(id);
    if (!kb) return;
    std::wstring q = trW(L"删除键盘皮肤「", L"Delete the keyboard skin “", L"Excluir a skin de teclado “") + utf8ToWide(kb->name)
        + trW(L"」？\n\n会从键盘皮肤文件夹里删除这个文件，删除后需要重新导入。",
              L"”?\n\nThe file will be removed from the keyboard skins folder; you'll have to import it again to get it back.",
              L"”?\n\nO arquivo será apagado da pasta de skins de teclado; para usá-la de novo, será preciso importá-la outra vez.");
    if (MessageBoxW(app.menuOwnerHwnd, q.c_str(), L"KuroCat", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST | MB_SETFOREGROUND | MB_DEFBUTTON2) != IDYES) return;
    std::string removing = id;
    if (!app.kbLibrary.remove(removing)) {
        MessageBoxW(app.menuOwnerHwnd, (trW(L"无法删除 ", L"Couldn't delete ", L"Não foi possível excluir ") + KeyboardSkinLibrary::directory() + L"\\" + utf8ToWide(removing) + L".kurokb").c_str(),
                    trW(L"删除失败", L"Delete Failed", L"Falha ao excluir"), MB_OK | MB_ICONWARNING | MB_TOPMOST);
        return;
    }
    if (app.settings.customKeyboard == removing) { app.settings.customKeyboard.clear(); app.settings.saveToRegistry(); }
    refreshDressingRoom(app);
}

namespace settings_ui {
enum CtrlId : int {
    ID_COAT_COMBO = 2001,
    ID_BTN_DRESSING,
    ID_BTN_RENAME_SKIN,
    ID_BTN_DEL_SKIN,
    ID_BTN_BATCH_DEL_SKIN,
    ID_EYE_COMBO,
    ID_KB_COMBO,
    ID_BTN_IMPORT_KB,
    ID_SCALE_SCROLL,
    ID_SCALE_VAL,
    ID_ROT_SCROLL,
    ID_ROT_VAL,
    ID_BTN_ROT_ZERO,
    ID_CHK_FLIP,
    ID_CHK_OUTLINE,
    ID_BTN_RESET_POS,
    ID_CHK_DRAG_ANIM,
    ID_CHK_ANGER,
    ID_CHK_PET,
    ID_CHK_BADGES,
    ID_CHK_MOUSE_BADGES,
    ID_CHK_LEGENDS,
    ID_CHK_MARQUEE,
    ID_CHK_LEVEL_KEYBOARD,
    ID_TYPING_LIMB_COMBO,
    ID_TABLET_COMBO,
    ID_CHK_TOPMOST,
    ID_CHK_AUTOHIDE_FS,
    ID_CHK_AUTO_HEADPHONES,
    ID_CHK_CLICKTHROUGH,
    ID_CHK_AUTOSTART,
    ID_CHK_RUN_ADMIN,
    ID_BTN_RELAUNCH_ADMIN,
    ID_OBS_COMBO,
    ID_CHK_OBS_HIDE,
    ID_LANG_COMBO,
    ID_BTN_SKINS_DIR,
    ID_BTN_KB_DIR,
    ID_BTN_EDIT_MODE,
    ID_SKINS_DIR_PATH,
    ID_BTN_CHOOSE_SKINS_DIR,
    ID_BTN_OPEN_SKINS_DIR,
    ID_BTN_RESET_SKINS_DIR
};

bool isWindowsDarkMode() {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS) return false;
    DWORD val = 1, sz = sizeof(val);
    bool dark = (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&val, &sz) == ERROR_SUCCESS && val == 0);
    RegCloseKey(hKey);
    return dark;
}

void applyDwmDarkTitleBar(HWND hwnd, bool dark) {
    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (!dwm) return;
    using Fn = HRESULT (WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
    auto fn = (Fn)GetProcAddress(dwm, "DwmSetWindowAttribute");
    if (fn) {
        BOOL v = dark ? TRUE : FALSE;
        // 20 = DWMWA_USE_IMMERSIVE_DARK_MODE (Win10 2004+ / Win11); 19 = older Win10 1903/1909
        if (FAILED(fn(hwnd, 20, &v, sizeof(v)))) fn(hwnd, 19, &v, sizeof(v));
    }
    FreeLibrary(dwm);
}

void rebuildCombosAndSync(AppState& app) {
    HWND hwnd = app.settingsWnd;
    if (!hwnd) return;
    app.settingsUpdating = true;
    auto tr = [&](const wchar_t* zh, const wchar_t* eng, const wchar_t* pt) -> const wchar_t* { return app.settings.tr(zh, eng, pt); };

    // Coat / Skin combo (6 built-in + imported)
    HWND coatCb = GetDlgItem(hwnd, ID_COAT_COMBO);
    if (coatCb) {
        SendMessageW(coatCb, CB_RESETCONTENT, 0, 0);
        const wchar_t* builtins[6] = {
            tr(L"\u7ECF\u5178\u5C0F\u9ED1\u732B", L"Classic Black Cat", L"Gatinho preto clássico"),
            tr(L"\u6316\u7164\u66B9\u7F57\u732B", L"Siamese Cat", L"Gato siamês"),
            tr(L"\u7F8E\u77ED\u94F6\u864E\u6591", L"Silver Tabby Shorthair", L"Pelo curto tigrado prateado"),
            tr(L"\u6A58\u767D\u732B", L"Orange & White Cat", L"Gato laranja e branco"),
            tr(L"\u5446\u732B", L"Dazed Cat", L"Gato atordoado"),
            tr(L"\u72F8\u82B1\u732B", L"Tabby Cat", L"Gato tigrado")
        };
        for (int i = 0; i < 6; ++i) SendMessageW(coatCb, CB_ADDSTRING, 0, (LPARAM)builtins[i]);
        int sel = (int)app.settings.coatTheme;
        for (size_t i = 0; i < app.skins.skins.size(); ++i) {
            SendMessageW(coatCb, CB_ADDSTRING, 0, (LPARAM)utf8ToWide(app.skins.displayName(app.skins.skins[i])).c_str());
            if (!app.settings.customSkin.empty() && app.skins.skins[i].id == app.settings.customSkin) {
                sel = 6 + (int)i;
            }
        }
        SendMessageW(coatCb, CB_SETCURSEL, (WPARAM)sel, 0);
        bool hasCustom = !app.settings.customSkin.empty() && app.skins.find(app.settings.customSkin) != nullptr;
        if (HWND hRen = GetDlgItem(hwnd, ID_BTN_RENAME_SKIN)) EnableWindow(hRen, hasCustom ? TRUE : FALSE);
        if (HWND hDel = GetDlgItem(hwnd, ID_BTN_DEL_SKIN)) {
            EnableWindow(hDel, hasCustom ? TRUE : FALSE);
            InvalidateRect(hDel, nullptr, TRUE);
        }
        if (HWND hBatch = GetDlgItem(hwnd, ID_BTN_BATCH_DEL_SKIN)) {
            EnableWindow(hBatch, !app.skins.skins.empty() ? TRUE : FALSE);
        }
    }

    // Eye combo
    HWND eyeCb = GetDlgItem(hwnd, ID_EYE_COMBO);
    if (eyeCb) {
        SendMessageW(eyeCb, CB_RESETCONTENT, 0, 0);
        const wchar_t* eyes[7] = {
            tr(L"\u8367\u5149\u9EC4\u7EFF", L"Neon Chartreuse", L"Verde-limão neon"),
            tr(L"\u66B9\u7F57\u5B9D\u77F3\u84DD", L"Siamese Ice Blue", L"Azul siamês"),
            tr(L"\u7425\u73C0\u91D1\u77B3", L"Warm Amber", L"Âmbar quente"),
            tr(L"\u51B0\u6676\u6D45\u84DD", L"Ice Cyan", L"Ciano gelo"),
            tr(L"\u67D4\u548C\u7425\u73C0", L"Soft Amber", L"Âmbar suave"),
            tr(L"\u6DE1\u91D1\u9752\u7EFF", L"Golden Hazel", L"Avelã dourado"),
            tr(L"\u6A44\u6984\u91D1", L"Olive Gold", L"Dourado oliva")
        };
        for (int i = 0; i < 7; ++i) SendMessageW(eyeCb, CB_ADDSTRING, 0, (LPARAM)eyes[i]);
        SendMessageW(eyeCb, CB_SETCURSEL, (WPARAM)(int)app.settings.eyeTheme, 0);
    }

    // Keyboard combo (3 built-in + imported)
    app.kbLibrary.reload();
    HWND kbCb = GetDlgItem(hwnd, ID_KB_COMBO);
    if (kbCb) {
        SendMessageW(kbCb, CB_RESETCONTENT, 0, 0);
        const wchar_t* kbs[3] = {
            tr(L"\u7ECF\u5178\u5976\u767D\u6696\u9EC4", L"Classic Milk White", L"Branco leite clássico"),
            tr(L"\u6697\u591C\u6781\u5BA2\u9ED1", L"Midnight Hacker Black", L"Preto hacker da meia-noite"),
            tr(L"\u590D\u53E4\u5976\u6CB9\u7070", L"Retro Cream Gray", L"Cinza creme retrô")
        };
        for (int i = 0; i < 3; ++i) SendMessageW(kbCb, CB_ADDSTRING, 0, (LPARAM)kbs[i]);
        int sel = (int)app.settings.keyboardTheme;
        for (size_t i = 0; i < app.kbLibrary.skins.size(); ++i) {
            SendMessageW(kbCb, CB_ADDSTRING, 0, (LPARAM)utf8ToWide(app.kbLibrary.skins[i].name).c_str());
            if (!app.settings.customKeyboard.empty() && app.kbLibrary.skins[i].id == app.settings.customKeyboard) {
                sel = 3 + (int)i;
            }
        }
        SendMessageW(kbCb, CB_SETCURSEL, (WPARAM)sel, 0);
    }

    // Scale & Rotation scrollbars
    int scalePct = std::clamp((int)std::lround(app.settings.scale * 100.0f), 45, 250);
    SetScrollPos(GetDlgItem(hwnd, ID_SCALE_SCROLL), SB_CTL, scalePct, TRUE);
    wchar_t sBuf[32] = {};
    swprintf_s(sBuf, L"%d%%", scalePct);
    SetWindowTextW(GetDlgItem(hwnd, ID_SCALE_VAL), sBuf);

    int rotDeg = std::clamp((int)std::lround(app.settings.rotationDeg), -180, 180);
    SetScrollPos(GetDlgItem(hwnd, ID_ROT_SCROLL), SB_CTL, rotDeg, TRUE);
    wchar_t rBuf[32] = {};
    swprintf_s(rBuf, L"%d\u00B0", rotDeg);
    SetWindowTextW(GetDlgItem(hwnd, ID_ROT_VAL), rBuf);

    auto setChk = [&](int id, bool v) {
        SendMessageW(GetDlgItem(hwnd, id), BM_SETCHECK, v ? BST_CHECKED : BST_UNCHECKED, 0);
    };
    setChk(ID_CHK_FLIP, app.settings.flipHorizontal);
    setChk(ID_CHK_OUTLINE, app.settings.whiteOutline);
    setChk(ID_CHK_DRAG_ANIM, app.settings.enableDragAnimation);
    setChk(ID_CHK_ANGER, app.settings.enableAnger);
    setChk(ID_CHK_PET, app.settings.petInteraction);
    setChk(ID_CHK_BADGES, app.settings.showKeyBadges);
    setChk(ID_CHK_MOUSE_BADGES, app.settings.showMouseBadges);
    setChk(ID_CHK_LEGENDS, app.settings.showKeyLegends);
    setChk(ID_CHK_MARQUEE, app.settings.keyboardMarquee);
    setChk(ID_CHK_LEVEL_KEYBOARD, app.settings.levelKeyboard);
    setChk(ID_CHK_TOPMOST, app.settings.alwaysOnTop);
    setChk(ID_CHK_AUTOHIDE_FS, app.settings.autoHideFullscreen);
    setChk(ID_CHK_AUTO_HEADPHONES, app.settings.autoHeadphonesOnMusic);
    setChk(ID_CHK_CLICKTHROUGH, app.settings.fullClickThrough);
    setChk(ID_CHK_AUTOSTART, isAutoStartEnabled());
    setChk(ID_CHK_RUN_ADMIN, app.settings.runAsAdmin);
    setChk(ID_CHK_OBS_HIDE, app.settings.obsHideDesktop);
    EnableWindow(GetDlgItem(hwnd, ID_CHK_OBS_HIDE), app.settings.obsCaptureMode != 0);
    EnableWindow(GetDlgItem(hwnd, ID_BTN_RELAUNCH_ADMIN), !app.isElevated);

    int limbIdx = app.settings.footEasterEgg
        ? (app.settings.tailEasterEgg ? 0 : 1)
        : (app.settings.tailEasterEgg ? 2 : 3);
    SendMessageW(GetDlgItem(hwnd, ID_TYPING_LIMB_COMBO), CB_SETCURSEL, (WPARAM)limbIdx, 0);
    SendMessageW(GetDlgItem(hwnd, ID_TABLET_COMBO), CB_SETCURSEL, (WPARAM)app.settings.tabletMode, 0);
    SendMessageW(GetDlgItem(hwnd, ID_OBS_COMBO), CB_SETCURSEL, (WPARAM)app.settings.obsCaptureMode, 0);
    SendMessageW(GetDlgItem(hwnd, ID_LANG_COMBO), CB_SETCURSEL, (WPARAM)std::clamp(app.settings.language, 0, 2), 0);

    if (HWND hDir = GetDlgItem(hwnd, ID_SKINS_DIR_PATH)) {
        SetWindowTextW(hDir, SkinLibrary::dir().c_str());
    }
    if (HWND hResetDir = GetDlgItem(hwnd, ID_BTN_RESET_SKINS_DIR)) {
        EnableWindow(hResetDir, SkinLibrary::isUsingCustomDir() ? TRUE : FALSE);
    }

    app.settingsUpdating = false;
}

bool pickFolderDialog(HWND owner, const wchar_t* title, const std::wstring& initialDir, std::wstring& outPath) {
    outPath.clear();
    using CoInitExFn = HRESULT (WINAPI*)(LPVOID, DWORD);
    using CoCreateInstFn = HRESULT (WINAPI*)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);
    using CoTaskMemFreeFn = void (WINAPI*)(LPVOID);
    using CoUninitFn = void (WINAPI*)();
    if (HMODULE ole = LoadLibraryW(L"ole32.dll")) {
        auto pCoInit = (CoInitExFn)(void*)GetProcAddress(ole, "CoInitializeEx");
        auto pCoCreate = (CoCreateInstFn)(void*)GetProcAddress(ole, "CoCreateInstance");
        auto pCoFree = (CoTaskMemFreeFn)(void*)GetProcAddress(ole, "CoTaskMemFree");
        auto pCoUninit = (CoUninitFn)(void*)GetProcAddress(ole, "CoUninitialize");
        if (pCoInit && pCoCreate && pCoFree && pCoUninit) {
            static const GUID kCLSID_FileOpenDialog = {0xDC1C5A9C, 0xE88A, 0x4DDE, {0xA5, 0xA1, 0x60, 0xF8, 0x2A, 0x20, 0xAE, 0xF7}};
            static const GUID kIID_IFileDialog      = {0x42F85136, 0xDB7E, 0x439C, {0x85, 0xF1, 0xE4, 0x07, 0x5D, 0x13, 0x5F, 0xC8}};
            static const GUID kIID_IShellItem       = {0x43826D1E, 0xE718, 0x42EE, {0xBC, 0x55, 0xA1, 0xE2, 0x61, 0xC3, 0x7B, 0xFE}};
            HRESULT hrInit = pCoInit(nullptr, 0x2 | 0x4);
            bool needUninit = SUCCEEDED(hrInit);
            if (SUCCEEDED(hrInit) || hrInit == RPC_E_CHANGED_MODE) {
                IFileDialog* pfd = nullptr;
                if (SUCCEEDED(pCoCreate(kCLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, kIID_IFileDialog, (void**)&pfd)) && pfd) {
                    DWORD opts = 0;
                    if (SUCCEEDED(pfd->GetOptions(&opts))) {
                        pfd->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
                    }
                    if (title) pfd->SetTitle(title);
                    if (!initialDir.empty()) {
                        using SHCreateItemFn = HRESULT (WINAPI*)(PCWSTR, IBindCtx*, REFIID, void**);
                        if (HMODULE sh32 = GetModuleHandleW(L"shell32.dll")) {
                            auto pCreateItem = (SHCreateItemFn)(void*)GetProcAddress(sh32, "SHCreateItemFromParsingName");
                            IShellItem* initItem = nullptr;
                            if (pCreateItem && SUCCEEDED(pCreateItem(initialDir.c_str(), nullptr, kIID_IShellItem, (void**)&initItem)) && initItem) {
                                pfd->SetFolder(initItem);
                                initItem->Release();
                            }
                        }
                    }
                    bool picked = false;
                    if (SUCCEEDED(pfd->Show(owner))) {
                        IShellItem* item = nullptr;
                        if (SUCCEEDED(pfd->GetResult(&item)) && item) {
                            PWSTR psz = nullptr;
                            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &psz)) && psz) {
                                outPath = psz;
                                pCoFree(psz);
                                picked = !outPath.empty();
                            }
                            item->Release();
                        }
                    }
                    pfd->Release();
                    if (needUninit) pCoUninit();
                    FreeLibrary(ole);
                    return picked;
                }
                if (needUninit) pCoUninit();
            }
        }
        FreeLibrary(ole);
    }
    BROWSEINFOW bi = {};
    bi.hwndOwner = owner;
    bi.lpszTitle = title;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return false;
    wchar_t buf[MAX_PATH] = {0};
    bool ok = SHGetPathFromIDListW(pidl, buf) != FALSE && buf[0] != L'\0';
    if (ok) outPath = buf;
    if (HMODULE ole = LoadLibraryW(L"ole32.dll")) {
        auto pCoFree = (CoTaskMemFreeFn)(void*)GetProcAddress(ole, "CoTaskMemFree");
        if (pCoFree) pCoFree(pidl);
        FreeLibrary(ole);
    }
    return ok;
}
} // namespace settings_ui

void showSettingsWindow(AppState& app);

void refreshSettingsWindow(AppState& app) {
    if (app.settingsWnd && IsWindow(app.settingsWnd)) {
        settings_ui::rebuildCombosAndSync(app);
    }
}

// Switches the UI language (0 = Chinese, 1 = English, 2 = Portuguese), persists it and
// re-labels whatever is open: tray tooltip, settings window (rebuilt) and the dressing room.
void applyUiLanguage(AppState& app, int lang) {
    lang = std::clamp(lang, 0, 2);
    app.settings.language = lang;
    kurocat::setUiLanguage(static_cast<kurocat::UiLanguage>(lang));
    app.settings.saveToRegistry();

    wcscpy_s(app.nid.szTip, trayTipText());
    NOTIFYICONDATAW tip = app.nid;
    tip.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &tip);

    if (app.dressingRoom) {
        SetWindowTextW(app.dressingRoom, trW(L"换装间", L"Dressing Room", L"Provador"));
        if (HWND btn = GetDlgItem(app.dressingRoom, IDC_DRESSING_CHOOSE))
            SetWindowTextW(btn, trW(L"选择文件…", L"Choose File…", L"Escolher arquivo…"));
        setDressingStatus(app, trW(L"支持拖入多个文件；坏掉的文件会告诉你哪里不对。",
                                   L"You can drop several files at once; broken files will tell you what's wrong.",
                                   L"Dá para soltar vários arquivos; se algum estiver com problema, você verá o motivo."), 0);
    }
    if (app.settingsWnd && IsWindow(app.settingsWnd)) {
        DestroyWindow(app.settingsWnd);
        showSettingsWindow(app);
    }
}

void applySkinsDirectory(AppState& app, const std::wstring& dirW) {
    std::wstring cleaned = SkinLibrary::trimmedDir(dirW);
    SkinLibrary::customDir() = cleaned;
    app.settings.customSkinsDir = wideToUtf8(cleaned);
    app.settings.saveToRegistry();
    app.skins.reload();
    refreshDressingRoom(app);
    refreshSettingsWindow(app);
}

LRESULT CALLBACK settingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* app = g_app;
    switch (msg) {
        case WM_ERASEBKGND:
            if (app && app->settingsBgBrush) {
                RECT rc = {};
                GetClientRect(hwnd, &rc);
                FillRect((HDC)wParam, &rc, app->settingsBgBrush);
                return TRUE;
            }
            break;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
            if (app && app->settingsBgBrush) {
                HDC hdc = (HDC)wParam;
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, app->settingsDark ? RGB(236, 238, 242) : RGB(28, 30, 34));
                return (LRESULT)app->settingsBgBrush;
            }
            break;
        case WM_HSCROLL: {
            if (!app || app->settingsUpdating) return 0;
            HWND sb = (HWND)lParam;
            int id = GetDlgCtrlID(sb);
            int cur = GetScrollPos(sb, SB_CTL);
            int lo = 0, hi = 100;
            GetScrollRange(sb, SB_CTL, &lo, &hi);
            int stepSmall = (id == settings_ui::ID_ROT_SCROLL) ? 1 : 2;
            int stepPage = (id == settings_ui::ID_ROT_SCROLL) ? 15 : 10;
            switch (LOWORD(wParam)) {
                case SB_LINELEFT: cur -= stepSmall; break;
                case SB_LINERIGHT: cur += stepSmall; break;
                case SB_PAGELEFT: cur -= stepPage; break;
                case SB_PAGERIGHT: cur += stepPage; break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: cur = (short)HIWORD(wParam); break;
                default: return 0;
            }
            cur = std::clamp(cur, lo, hi);
            SetScrollPos(sb, SB_CTL, cur, TRUE);
            if (id == settings_ui::ID_SCALE_SCROLL) {
                app->settings.scale = (float)cur / 100.0f;
                applyWindowScaleAndPosition(*app, true, true);
                wchar_t buf[32] = {};
                swprintf_s(buf, L"%d%%", cur);
                SetWindowTextW(GetDlgItem(hwnd, settings_ui::ID_SCALE_VAL), buf);
            } else if (id == settings_ui::ID_ROT_SCROLL) {
                if (std::abs(cur) <= 3) cur = 0;
                else if (std::abs(cur - 90) <= 3) cur = 90;
                else if (std::abs(cur + 90) <= 3) cur = -90;
                SetScrollPos(sb, SB_CTL, cur, TRUE);
                app->settings.rotationDeg = (float)cur;
                app->settings.saveToRegistry();
                wchar_t buf[32] = {};
                swprintf_s(buf, L"%d\u00B0", cur);
                SetWindowTextW(GetDlgItem(hwnd, settings_ui::ID_ROT_VAL), buf);
            }
            return 0;
        }
        case WM_DRAWITEM: {
            auto* dis = (DRAWITEMSTRUCT*)lParam;
            if (dis && dis->CtlID == settings_ui::ID_BTN_DEL_SKIN) {
                bool disabled = (dis->itemState & ODS_DISABLED) != 0;
                bool pressed = (dis->itemState & ODS_SELECTED) != 0;
                COLORREF bgCol = disabled ? RGB(224, 224, 224) : (pressed ? RGB(175, 35, 35) : RGB(217, 56, 56));
                COLORREF txtCol = disabled ? RGB(145, 145, 145) : RGB(255, 255, 255);
                HBRUSH br = CreateSolidBrush(bgCol);
                FillRect(dis->hDC, &dis->rcItem, br);
                DeleteObject(br);
                HPEN pen = CreatePen(PS_SOLID, 1, disabled ? RGB(195, 195, 195) : RGB(165, 30, 30));
                HGDIOBJ oldPen = SelectObject(dis->hDC, pen);
                HGDIOBJ oldBr = SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
                Rectangle(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom);
                SelectObject(dis->hDC, oldPen);
                SelectObject(dis->hDC, oldBr);
                DeleteObject(pen);
                SetBkMode(dis->hDC, TRANSPARENT);
                SetTextColor(dis->hDC, txtCol);
                wchar_t txt[64] = {};
                GetWindowTextW(dis->hwndItem, txt, 63);
                DrawTextW(dis->hDC, txt, -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                return TRUE;
            }
            break;
        }
        case WM_COMMAND: {
            if (!app || app->settingsUpdating) return 0;
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);
            using namespace settings_ui;
            if (code == CBN_SELCHANGE) {
                int sel = (int)SendMessageW((HWND)lParam, CB_GETCURSEL, 0, 0);
                if (sel < 0) return 0;
                if (id == ID_COAT_COMBO) {
                    app->settings.overrideSkinIris = false;
                    if (sel < 6) {
                        app->settings.customSkin.clear();
                        app->settings.coatTheme = (CatCoatTheme)sel;
                        switch (app->settings.coatTheme) {
                            case CatCoatTheme::ClassicBlack: app->settings.eyeTheme = EyeColorTheme::NeonChartreuse; break;
                            case CatCoatTheme::Siamese:      app->settings.eyeTheme = EyeColorTheme::SiameseBlue; break;
                            case CatCoatTheme::Shorthair:    app->settings.eyeTheme = EyeColorTheme::ReferenceHazel; break;
                            case CatCoatTheme::OrangeWhite:  app->settings.eyeTheme = EyeColorTheme::WarmAmber; break;
                            case CatCoatTheme::Dazed:        app->settings.eyeTheme = EyeColorTheme::SoftAmber; break;
                            case CatCoatTheme::Tabby:        app->settings.eyeTheme = EyeColorTheme::OliveGold; break;
                        }
                    } else if ((size_t)(sel - 6) < app->skins.skins.size()) {
                        app->settings.customSkin = app->skins.skins[sel - 6].id;
                    }
                    app->settings.saveToRegistry();
                    refreshDressingRoom(*app);
                    refreshSettingsWindow(*app);
                } else if (id == ID_EYE_COMBO && sel <= (int)EyeColorTheme::OliveGold) {
                    app->settings.overrideSkinIris = true;
                    app->settings.eyeTheme = (EyeColorTheme)sel;
                    app->settings.saveToRegistry();
                } else if (id == ID_KB_COMBO) {
                    if (sel < 3) {
                        app->settings.customKeyboard.clear();
                        app->settings.keyboardTheme = (KeyboardTheme)sel;
                    } else if ((size_t)(sel - 3) < app->kbLibrary.skins.size()) {
                        app->settings.customKeyboard = app->kbLibrary.skins[sel - 3].id;
                    }
                    app->settings.saveToRegistry();
                } else if (id == ID_TYPING_LIMB_COMBO && sel <= 3) {
                    app->settings.footEasterEgg = (sel == 0 || sel == 1);
                    app->settings.tailEasterEgg = (sel == 0 || sel == 2);
                    app->settings.saveToRegistry();
                } else if (id == ID_TABLET_COMBO && sel <= 2) {
                    app->settings.tabletMode = sel;
                    app->settings.saveToRegistry();
                } else if (id == ID_OBS_COMBO && sel <= 2) {
                    app->settings.obsCaptureMode = sel;
                    if (sel == 0) app->settings.obsHideDesktop = false;
                    app->settings.saveToRegistry();
                    applyOBSMode(*app);
                    refreshSettingsWindow(*app);
                } else if (id == ID_LANG_COMBO && sel >= 0 && sel <= 2) {
                    if (sel != app->settings.language) {
                        applyUiLanguage(*app, sel);   // rebuilds this window in the new language
                    }
                }
                return 0;
            }
            if (code == BN_CLICKED) {
                auto isChk = [&](int cid) { return SendMessageW(GetDlgItem(hwnd, cid), BM_GETCHECK, 0, 0) == BST_CHECKED; };
                switch (id) {
                    case ID_BTN_DRESSING: showDressingRoom(*app); break;
                    case ID_BTN_RENAME_SKIN:
                        if (!app->settings.customSkin.empty()) promptRenameSkin(*app, app->settings.customSkin);
                        break;
                    case ID_BTN_DEL_SKIN:
                        if (!app->settings.customSkin.empty()) confirmDeleteSkin(*app, app->settings.customSkin);
                        break;
                    case ID_BTN_BATCH_DEL_SKIN:
                        confirmBatchDeleteSkins(*app);
                        break;
                    case ID_BTN_IMPORT_KB: chooseKeyboardSkinFile(*app); break;
                    case ID_BTN_ROT_ZERO:
                        app->settings.rotationDeg = 0.0f;
                        app->settings.saveToRegistry();
                        refreshSettingsWindow(*app);
                        break;
                    case ID_BTN_RESET_POS:
                        app->settings.scale = 1.0f;
                        app->settings.rotationDeg = 0.0f;
                        app->settings.flipHorizontal = false;
                        app->settings.posX = -99999;
                        app->settings.posY = -99999;
                        applyWindowScaleAndPosition(*app, false, true);
                        refreshSettingsWindow(*app);
                        break;
                    case ID_BTN_EDIT_MODE:
                        app->settings.isEditMode = !app->settings.isEditMode;
                        break;
                    case ID_CHK_FLIP:
                        app->settings.flipHorizontal = isChk(id);
                        app->settings.saveToRegistry();
                        break;
                    case ID_CHK_OUTLINE:
                        app->settings.whiteOutline = isChk(id);
                        app->settings.saveToRegistry();
                        break;
                    case ID_CHK_DRAG_ANIM:
                        app->settings.enableDragAnimation = isChk(id);
                        app->settings.saveToRegistry();
                        break;
                    case ID_CHK_ANGER:
                        app->settings.enableAnger = isChk(id);
                        app->settings.saveToRegistry();
                        break;
                    case ID_CHK_PET:
                        app->settings.petInteraction = isChk(id);
                        app->settings.saveToRegistry();
                        break;
                    case ID_CHK_BADGES:
                        app->settings.showKeyBadges = isChk(id);
                        app->settings.saveToRegistry();
                        break;
                    case ID_CHK_MOUSE_BADGES:
                        app->settings.showMouseBadges = isChk(id);
                        app->settings.saveToRegistry();
                        break;
                    case ID_CHK_LEGENDS:
                        app->settings.showKeyLegends = isChk(id);
                        app->settings.saveToRegistry();
                        break;
                    case ID_CHK_MARQUEE:
                        app->settings.keyboardMarquee = isChk(id);
                        app->settings.saveToRegistry();
                        break;
                    case ID_CHK_LEVEL_KEYBOARD:
                        app->settings.levelKeyboard = isChk(id);
                        KeyboardLayout::levelKeyboard() = app->settings.levelKeyboard;
                        app->settings.saveToRegistry();
                        break;
                    case ID_CHK_TOPMOST:
                        app->settings.alwaysOnTop = isChk(id);
                        applyWindowScaleAndPosition(*app, false, true);
                        break;
                    case ID_CHK_AUTOHIDE_FS:
                        app->settings.autoHideFullscreen = isChk(id);
                        app->settings.saveToRegistry();
                        if (!app->settings.autoHideFullscreen && app->hiddenByFullscreen) {
                            app->hiddenByFullscreen = false;
                            if (app->isVisible) { ShowWindow(app->hwnd, SW_SHOWNOACTIVATE); assertTopmost(*app); }
                        }
                        app->nextHousekeepingTime = 0.0;
                        break;
                    case ID_CHK_AUTO_HEADPHONES:
                        app->settings.autoHeadphonesOnMusic = isChk(id);
                        if (!app->settings.autoHeadphonesOnMusic) app->physics.isMusicPlaying = false;
                        app->settings.saveToRegistry();
                        break;
                    case ID_CHK_CLICKTHROUGH:
                        app->settings.fullClickThrough = isChk(id);
                        app->settings.saveToRegistry();
                        if (app->settings.fullClickThrough) {
                            showTrayBalloon(*app,
                                app->settings.tr(L"KuroCat \u5DF2\u5F00\u542F\u5B8C\u5168\u9F20\u6807\u7A7F\u900F",
                                                 L"KuroCat full click-through is on",
                                                 L"KuroCat: ignorar cliques ativado"),
                                app->settings.tr(L"\u53D6\u6D88\u65B9\u5F0F\uFF1A\u6309 Ctrl+Alt+Shift+P\uFF0C\u6216\u6309\u4F4F Alt \u952E\u53F3\u952E\u5C0F\u732B\uFF0C\u6216\u70B9\u53F3\u4E0B\u89D2\u6258\u76D8\u56FE\u6807",
                                                 L"To turn it off: press Ctrl+Alt+Shift+P, hold Alt and right-click the cat, or click the tray icon.",
                                                 L"Para desativar: pressione Ctrl+Alt+Shift+P, segure Alt e clique com o botão direito no gato ou clique no ícone da bandeja."));
                        }
                        break;
                    case ID_CHK_AUTOSTART:
                        setAutoStartEnabled(isChk(id));
                        break;
                    case ID_CHK_RUN_ADMIN:
                        app->settings.runAsAdmin = isChk(id);
                        app->settings.saveToRegistry();
                        if (app->settings.runAsAdmin && !app->isElevated && relaunchSelfElevated(L"--takeover --no-elevate")) {
                            quitApp(*app);
                        }
                        break;
                    case ID_BTN_RELAUNCH_ADMIN:
                        if (!app->isElevated && relaunchSelfElevated(L"--takeover --no-elevate")) quitApp(*app);
                        break;
                    case ID_CHK_OBS_HIDE:
                        app->settings.obsHideDesktop = isChk(id);
                        app->settings.saveToRegistry();
                        applyOBSMode(*app);
                        break;
                    case ID_BTN_SKINS_DIR:
                    case ID_BTN_OPEN_SKINS_DIR:
                        SkinLibrary::ensureDir();
                        ShellExecuteW(nullptr, L"open", SkinLibrary::dir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                        break;
                    case ID_BTN_CHOOSE_SKINS_DIR: {
                        std::wstring chosen;
                        if (settings_ui::pickFolderDialog(hwnd,
                                app->settings.tr(L"\u9009\u62E9\u76AE\u80A4\u6587\u4EF6\u5939", L"Choose Skins Folder", L"Escolher pasta de skins"),
                                SkinLibrary::dir(), chosen)) {
                            applySkinsDirectory(*app, chosen);
                        }
                        break;
                    }
                    case ID_BTN_RESET_SKINS_DIR:
                        applySkinsDirectory(*app, L"");
                        break;
                    case ID_BTN_KB_DIR:
                        KeyboardSkinLibrary::ensureDir();
                        ShellExecuteW(nullptr, L"open", KeyboardSkinLibrary::directory().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                        break;
                }
            }
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (app && app->settingsWnd == hwnd) {
                app->settingsWnd = nullptr;
                if (app->settingsFont) { DeleteObject(app->settingsFont); app->settingsFont = nullptr; }
                if (app->settingsBoldFont) { DeleteObject(app->settingsBoldFont); app->settingsBoldFont = nullptr; }
                if (app->settingsBgBrush) { DeleteObject(app->settingsBgBrush); app->settingsBgBrush = nullptr; }
            }
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void showSettingsWindow(AppState& app) {
    if (app.settingsWnd && IsWindow(app.settingsWnd)) {
        refreshSettingsWindow(app);
        ShowWindow(app.settingsWnd, SW_SHOWNORMAL);
        SetForegroundWindow(app.settingsWnd);
        return;
    }
    HINSTANCE inst = GetModuleHandleW(nullptr);
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = settingsWndProc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = SETTINGS_WINDOW_CLASS;
        registered = RegisterClassExW(&wc) != 0;
    }

    HDC screen = GetDC(nullptr);
    float dpiScale = GetDeviceCaps(screen, LOGPIXELSY) / 96.0f;
    ReleaseDC(nullptr, screen);

    const int W = 540, H = 740;
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    RECT frameRc = { 0, 0, 0, 0 };
    AdjustWindowRectEx(&frameRc, style, FALSE, 0);
    int frameW = frameRc.right - frameRc.left, frameH = frameRc.bottom - frameRc.top;
    RECT work = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int maxClientW = std::max(320, (int)(work.right - work.left) - frameW - 24);
    int maxClientH = std::max(360, (int)(work.bottom - work.top) - frameH - 24);
    float k = std::min({ dpiScale, (float)maxClientW / (float)W, (float)maxClientH / (float)H });
    if (k < 0.65f) k = 0.65f;
    auto px = [&](int v) { return (int)std::lround(v * k); };

    app.settingsDark = settings_ui::isWindowsDarkMode();
    app.settingsBgBrush = CreateSolidBrush(app.settingsDark ? RGB(32, 32, 36) : RGB(246, 246, 248));
    app.settingsFont = CreateFontW(-px(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    app.settingsBoldFont = CreateFontW(-px(13), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    auto tr = [&](const wchar_t* zh, const wchar_t* eng, const wchar_t* pt) -> const wchar_t* { return app.settings.tr(zh, eng, pt); };

    RECT rc = { 0, 0, px(W), px(H) };
    AdjustWindowRectEx(&rc, style, FALSE, 0);
    int winW = rc.right - rc.left, winH = rc.bottom - rc.top;
    int posX = std::max((int)work.left, (int)(work.left + work.right - winW) / 2);
    int posY = std::max((int)work.top, (int)(work.top + work.bottom - winH) / 2);
    HWND win = CreateWindowExW(0, SETTINGS_WINDOW_CLASS,
        tr(L"KuroCat \u8BBE\u7F6E", L"KuroCat Settings", L"Configurações do KuroCat"), style,
        posX, posY,
        winW, winH, nullptr, nullptr, inst, nullptr);
    if (!win) return;
    app.settingsWnd = win;
    settings_ui::applyDwmDarkTitleBar(win, app.settingsDark);

    auto addCtrl = [&](const wchar_t* cls, const wchar_t* txt, DWORD st, int x, int y, int w, int h, int id, bool bold = false) -> HWND {
        HWND c = CreateWindowExW(0, cls, txt, WS_CHILD | WS_VISIBLE | st,
            px(x), px(y), px(w), px(h), win, (HMENU)(INT_PTR)id, inst, nullptr);
        if (c) SendMessageW(c, WM_SETFONT, (WPARAM)(bold ? app.settingsBoldFont : app.settingsFont), TRUE);
        return c;
    };

    using namespace settings_ui;
    // Section 1: Appearance & Orientation
    addCtrl(L"BUTTON", tr(L"\u5916\u89C2\u4E0E\u65B9\u5411", L"Appearance & Orientation", L"Aparência e orientação"), BS_GROUPBOX, 14, 10, 512, 264, 0, true);

    addCtrl(L"STATIC", tr(L"\u732B\u54AA\u76AE\u80A4", L"Cat Skin", L"Skin do gato"), SS_LEFT, 28, 38, 76, 20, 0);
    addCtrl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 110, 34, 148, 220, ID_COAT_COMBO);
    addCtrl(L"BUTTON", tr(L"\u6362\u88C5\u95F4", L"Wardrobe", L"Provador"), BS_PUSHBUTTON | WS_TABSTOP, 262, 34, 60, 24, ID_BTN_DRESSING);
    addCtrl(L"BUTTON", tr(L"\u6539\u540D", L"Name", L"Nome"), BS_PUSHBUTTON | WS_TABSTOP, 326, 34, 48, 24, ID_BTN_RENAME_SKIN);
    addCtrl(L"BUTTON", tr(L"\U0001F5D1\u5220", L"Del", L"\U0001F5D1"), BS_OWNERDRAW | WS_TABSTOP, 378, 34, 52, 24, ID_BTN_DEL_SKIN, true);
    addCtrl(L"BUTTON", tr(L"\u6279\u91CF\u5220\u9664\u2026", L"Batch Del...", L"Várias…"), BS_PUSHBUTTON | WS_TABSTOP, 434, 34, 78, 24, ID_BTN_BATCH_DEL_SKIN);

    addCtrl(L"STATIC", tr(L"\u76AE\u80A4\u76EE\u5F55", L"Skins Dir", L"Pasta"), SS_LEFT, 28, 70, 76, 20, 0);
    addCtrl(L"STATIC", L"", SS_LEFT | SS_PATHELLIPSIS | SS_NOPREFIX, 110, 70, 178, 20, ID_SKINS_DIR_PATH);
    addCtrl(L"BUTTON", tr(L"\u9009\u62E9\u6587\u4EF6\u5939\u2026", L"Choose...", L"Escolher…"), BS_PUSHBUTTON | WS_TABSTOP, 292, 66, 100, 24, ID_BTN_CHOOSE_SKINS_DIR);
    addCtrl(L"BUTTON", tr(L"\u6253\u5F00", L"Open", L"Abrir"), BS_PUSHBUTTON | WS_TABSTOP, 396, 66, 50, 24, ID_BTN_OPEN_SKINS_DIR);
    addCtrl(L"BUTTON", tr(L"\u9ED8\u8BA4", L"Reset", L"Padrão"), BS_PUSHBUTTON | WS_TABSTOP, 450, 66, 62, 24, ID_BTN_RESET_SKINS_DIR);

    addCtrl(L"STATIC", tr(L"\u77B3\u8272\u5207\u6362", L"Eye Color", L"Olhos"), SS_LEFT, 28, 102, 76, 20, 0);
    addCtrl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 110, 98, 290, 220, ID_EYE_COMBO);
    addCtrl(L"BUTTON", tr(L"\u6846\u9009\u8C03\u6574", L"Edit Box", L"Redimensionar"), BS_PUSHBUTTON | WS_TABSTOP, 410, 98, 102, 24, ID_BTN_EDIT_MODE);

    addCtrl(L"STATIC", tr(L"\u952E\u76D8\u914D\u8272", L"Keyboard", L"Teclado"), SS_LEFT, 28, 134, 76, 20, 0);
    addCtrl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 110, 130, 290, 220, ID_KB_COMBO);
    addCtrl(L"BUTTON", tr(L"\u5BFC\u5165\u952E\u76D8\u2026", L"Import...", L"Importar…"), BS_PUSHBUTTON | WS_TABSTOP, 410, 130, 102, 24, ID_BTN_IMPORT_KB);

    addCtrl(L"STATIC", tr(L"\u7F29\u653E\u5927\u5C0F", L"Pet Scale", L"Tamanho"), SS_LEFT, 28, 168, 76, 20, 0);
    HWND scaleSb = addCtrl(L"SCROLLBAR", L"", SBS_HORZ | WS_TABSTOP, 110, 168, 290, 18, ID_SCALE_SCROLL);
    SetScrollRange(scaleSb, SB_CTL, 45, 250, FALSE);
    addCtrl(L"STATIC", L"100%", SS_RIGHT, 410, 168, 48, 20, ID_SCALE_VAL);

    addCtrl(L"STATIC", tr(L"\u671D\u5411\u89D2\u5EA6", L"Rotation", L"Rotação"), SS_LEFT, 28, 200, 76, 20, 0);
    HWND rotSb = addCtrl(L"SCROLLBAR", L"", SBS_HORZ | WS_TABSTOP, 110, 200, 290, 18, ID_ROT_SCROLL);
    SetScrollRange(rotSb, SB_CTL, -180, 180, FALSE);
    addCtrl(L"STATIC", L"0\u00B0", SS_RIGHT, 408, 200, 42, 20, ID_ROT_VAL);
    addCtrl(L"BUTTON", tr(L"\u5F52\u96F6", L"0\u00B0", L"0°"), BS_PUSHBUTTON | WS_TABSTOP, 458, 196, 54, 24, ID_BTN_ROT_ZERO);

    addCtrl(L"BUTTON", tr(L"\u6C34\u5E73\u7FFB\u8F6C", L"Flip Horizontal", L"Espelhar imagem"), BS_AUTOCHECKBOX | WS_TABSTOP, 28, 234, 140, 22, ID_CHK_FLIP);
    addCtrl(L"BUTTON", tr(L"\u767D\u8272\u8D34\u7EB8\u63CF\u8FB9", L"White Sticker Outline", L"Contorno branco (adesivo)"), BS_AUTOCHECKBOX | WS_TABSTOP, 180, 234, 180, 22, ID_CHK_OUTLINE);
    addCtrl(L"BUTTON", tr(L"\u91CD\u7F6E\u4F4D\u7F6E\u4E0E\u671D\u5411", L"Reset Pose", L"Redefinir posição"), BS_PUSHBUTTON | WS_TABSTOP, 384, 232, 128, 24, ID_BTN_RESET_POS);

    // Section 2: Interaction & Effects
    addCtrl(L"BUTTON", tr(L"\u4E92\u52A8\u4E0E\u7279\u6548", L"Interaction & Effects", L"Interação e efeitos"), BS_GROUPBOX, 14, 284, 512, 208, 0, true);
    addCtrl(L"BUTTON", tr(L"\u62CE\u8D77\u62D6\u62FD\u52A8\u753B", L"Scruff-Drag Lift Animation", L"Animação de erguer ao arrastar"), BS_AUTOCHECKBOX | WS_TABSTOP, 28, 310, 236, 22, ID_CHK_DRAG_ANIM);
    addCtrl(L"BUTTON", tr(L"\u9AD8\u901F\u6253\u5B57\u751F\u6C14\u8868\u60C5", L"Angry Expression on Fast Typing", L"Cara brava ao digitar rápido"), BS_AUTOCHECKBOX | WS_TABSTOP, 276, 310, 236, 22, ID_CHK_ANGER);
    addCtrl(L"BUTTON", tr(L"\u6478\u732B\u4E92\u52A8\u4E0E\u5495\u565C\u7279\u6548", L"Petting & Purr Sparkles", L"Carinho e brilho do ronronar"), BS_AUTOCHECKBOX | WS_TABSTOP, 28, 338, 236, 22, ID_CHK_PET);
    addCtrl(L"BUTTON", tr(L"\u663E\u793A\u6309\u952E\u6C14\u6CE1", L"Show Keyboard Key Badges", L"Mostrar balões das teclas"), BS_AUTOCHECKBOX | WS_TABSTOP, 276, 338, 236, 22, ID_CHK_BADGES);
    addCtrl(L"BUTTON", tr(L"\u663E\u793A\u9F20\u6807\u70B9\u51FB", L"Show Mouse Click Badges", L"Mostrar cliques do mouse"), BS_AUTOCHECKBOX | WS_TABSTOP, 28, 366, 236, 22, ID_CHK_MOUSE_BADGES);
    addCtrl(L"BUTTON", tr(L"\u663E\u793A\u952E\u76D8\u5B57\u7B26", L"Show Keycap Legends", L"Mostrar letras nas teclas"), BS_AUTOCHECKBOX | WS_TABSTOP, 276, 366, 236, 22, ID_CHK_LEGENDS);
    addCtrl(L"BUTTON", tr(L"\u952E\u76D8\u8DD1\u9A6C\u706F\u6D41\u5149", L"Keyboard RGB Marquee", L"Luz RGB animada no teclado"), BS_AUTOCHECKBOX | WS_TABSTOP, 28, 394, 236, 22, ID_CHK_MARQUEE);
    addCtrl(L"BUTTON", tr(L"\u952E\u76D8\u6C34\u5E73\u6446\u653E", L"Level Keyboard", L"Teclado nivelado"), BS_AUTOCHECKBOX | WS_TABSTOP, 276, 394, 236, 22, ID_CHK_LEVEL_KEYBOARD);

    addCtrl(L"STATIC", tr(L"\u591A\u952E\u6253\u5B57", L"Multi-Key", L"Várias teclas"), SS_LEFT, 28, 426, 90, 20, 0);
    HWND limbCb = addCtrl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 124, 422, 388, 140, ID_TYPING_LIMB_COMBO);
    SendMessageW(limbCb, CB_ADDSTRING, 0, (LPARAM)tr(L"\u540E\u811A + \u5C3E\u5DF4\uFF083\u952E\u540E\u811A / 4\u952E\u5C3E\u5DF4\uFF09", L"Hind Foot + Tail (3-Key / 4-Key)", L"Pata traseira + cauda (3 / 4 teclas)"));
    SendMessageW(limbCb, CB_ADDSTRING, 0, (LPARAM)tr(L"\u7528\u7B2C\u4E09\u53EA\u811A\u6253\u5B57\uFF08\u4EC5\u540E\u811A\uFF09", L"3rd Hind Foot Only", L"Só a pata traseira"));
    SendMessageW(limbCb, CB_ADDSTRING, 0, (LPARAM)tr(L"\u7528\u5C3E\u5DF4\u6253\u5B57\uFF08\u4EC5\u5C3E\u5DF4\uFF09", L"Tail Typing Only", L"Só a cauda"));
    SendMessageW(limbCb, CB_ADDSTRING, 0, (LPARAM)tr(L"\u53EA\u7528\u4E24\u53EA\u722A\u722A\u6253\u5B57", L"Two Front Paws Only", L"Só as duas patas da frente"));

    addCtrl(L"STATIC", tr(L"\u6570\u4F4D\u677F\u6A21\u5F0F", L"Tablet Mode", L"Mesa e caneta"), SS_LEFT, 28, 458, 90, 20, 0);
    HWND tabCb = addCtrl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 124, 454, 388, 120, ID_TABLET_COMBO);
    SendMessageW(tabCb, CB_ADDSTRING, 0, (LPARAM)tr(L"\u81EA\u52A8\u68C0\u6D4B\uFF08\u8FDE\u63A5\u6570\u4F4D\u677F\u65F6\u81EA\u52A8\u5207\u6362\uFF09", L"Auto-Detect (Switch When Connected)", L"Detectar automaticamente (ao conectar a mesa)"));
    SendMessageW(tabCb, CB_ADDSTRING, 0, (LPARAM)tr(L"\u59CB\u7EC8\u663E\u793A\u6570\u4F4D\u677F\u4E0E\u89E6\u63A7\u7B14", L"Always Show Tablet & Stylus", L"Sempre mostrar mesa e caneta"));
    SendMessageW(tabCb, CB_ADDSTRING, 0, (LPARAM)tr(L"\u59CB\u7EC8\u663E\u793A\u9F20\u6807", L"Always Show Mouse", L"Sempre mostrar mouse"));

    // Section 3: Window & System
    addCtrl(L"BUTTON", tr(L"\u7A97\u53E3\u4E0E\u7CFB\u7EDF", L"Window & System", L"Janela e sistema"), BS_GROUPBOX, 14, 502, 512, 224, 0, true);
    addCtrl(L"BUTTON", tr(L"\u7A97\u53E3\u7F6E\u9876\u663E\u793A", L"Always On Top", L"Sempre no topo"), BS_AUTOCHECKBOX | WS_TABSTOP, 28, 528, 236, 22, ID_CHK_TOPMOST);
    addCtrl(L"BUTTON", tr(L"\u5168\u5C4F\u5E94\u7528\u65F6\u81EA\u52A8\u9690\u85CF", L"Auto-Hide in Fullscreen", L"Ocultar em apps de tela cheia"), BS_AUTOCHECKBOX | WS_TABSTOP, 276, 528, 236, 22, ID_CHK_AUTOHIDE_FS);
    addCtrl(L"BUTTON", tr(L"\u5B8C\u5168\u9F20\u6807\u7A7F\u900F (Alt \u4E34\u65F6\u89E3\u9501)", L"Full Click-Through (Hold Alt)", L"Ignorar cliques (segure Alt)"), BS_AUTOCHECKBOX | WS_TABSTOP, 28, 556, 236, 22, ID_CHK_CLICKTHROUGH);
    addCtrl(L"BUTTON", tr(L"\u5F00\u673A\u81EA\u52A8\u542F\u52A8", L"Launch at Startup", L"Abrir ao iniciar o Windows"), BS_AUTOCHECKBOX | WS_TABSTOP, 276, 556, 236, 22, ID_CHK_AUTOSTART);
    addCtrl(L"BUTTON", tr(L"\u59CB\u7EC8\u4EE5\u7BA1\u7406\u5458\u8EAB\u4EFD\u8FD0\u884C", L"Always Run as Administrator", L"Sempre executar como admin"), BS_AUTOCHECKBOX | WS_TABSTOP, 28, 584, 236, 22, ID_CHK_RUN_ADMIN);
    addCtrl(L"BUTTON", app.isElevated ? tr(L"\u2713 \u5DF2\u7BA1\u7406\u5458\u8FD0\u884C", L"\u2713 Running as Admin", L"✓ Em modo admin")
                                      : tr(L"\u7BA1\u7406\u5458\u8EAB\u4EFD\u91CD\u542F", L"Restart as Admin", L"Reiniciar como admin"),
            BS_PUSHBUTTON | WS_TABSTOP, 276, 582, 148, 24, ID_BTN_RELAUNCH_ADMIN);
    addCtrl(L"BUTTON", tr(L"\u542C\u6B4C\u81EA\u52A8\u6234\u8033\u673A\uFF08\u97F3\u4E50/\u5A92\u4F53\u64AD\u653E\u5668\uFF09", L"Auto Headphones on Music", L"Fones automáticos ao ouvir música"), BS_AUTOCHECKBOX | WS_TABSTOP, 28, 612, 340, 22, ID_CHK_AUTO_HEADPHONES);

    addCtrl(L"STATIC", tr(L"OBS \u76F4\u64AD", L"OBS Capture", L"Captura OBS"), SS_LEFT, 28, 646, 78, 20, 0);
    HWND obsCb = addCtrl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 110, 642, 210, 120, ID_OBS_COMBO);
    SendMessageW(obsCb, CB_ADDSTRING, 0, (LPARAM)tr(L"\u5173\u95ED\uFF08\u6B63\u5E38\u60AC\u6D6E\uFF09", L"Off (Normal Overlay)", L"Desligado (normal)"));
    SendMessageW(obsCb, CB_ADDSTRING, 0, (LPARAM)tr(L"OBS \u900F\u660E\u7A97\u53E3\uFF08\u9700 WGC \u91C7\u96C6\uFF09", L"OBS Transparent (Needs WGC)", L"OBS transparente (WGC)"));
    SendMessageW(obsCb, CB_ADDSTRING, 0, (LPARAM)tr(L"OBS \u7EFF\u5E55\uFF08\u901A\u7528\u517C\u5BB9\uFF0C\u63A8\u8350\u76F4\u64AD\u59EC\uFF09", L"OBS Green Screen (Universal)", L"OBS tela verde (universal)"));
    addCtrl(L"BUTTON", tr(L"\u540E\u53F0\u6355\u83B7\uFF08\u9690\u85CF\u684C\u9762\uFF09", L"Hide on Desktop", L"Ocultar na tela"), BS_AUTOCHECKBOX | WS_TABSTOP, 332, 644, 180, 22, ID_CHK_OBS_HIDE);

    addCtrl(L"STATIC", tr(L"\u754C\u9762\u8BED\u8A00", L"Language", L"Idioma"), SS_LEFT, 28, 680, 78, 20, 0);
    HWND langCb = addCtrl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 110, 676, 150, 110, ID_LANG_COMBO);
    SendMessageW(langCb, CB_ADDSTRING, 0, (LPARAM)L"\u7B80\u4F53\u4E2D\u6587");
    SendMessageW(langCb, CB_ADDSTRING, 0, (LPARAM)L"English");
    SendMessageW(langCb, CB_ADDSTRING, 0, (LPARAM)L"Português");
    addCtrl(L"BUTTON", tr(L"\u76AE\u80A4\u76EE\u5F55", L"Skins Dir", L"Pasta de skins"), BS_PUSHBUTTON | WS_TABSTOP, 270, 676, 116, 24, ID_BTN_SKINS_DIR);
    addCtrl(L"BUTTON", tr(L"\u952E\u76D8\u76EE\u5F55", L"Keyboards Dir", L"Pasta de teclados"), BS_PUSHBUTTON | WS_TABSTOP, 392, 676, 120, 24, ID_BTN_KB_DIR);

    settings_ui::rebuildCombosAndSync(app);
    ShowWindow(win, SW_SHOWNORMAL);
    SetForegroundWindow(win);
}

namespace update_ui {
constexpr int ID_UPD_HEADING   = 3001;
constexpr int ID_UPD_BODY      = 3002;
constexpr int ID_UPD_SECONDARY = 3003;
#ifndef DM_GETDEFID
#define DM_GETDEFID (WM_USER + 0)
#endif
#ifndef DC_HASDEFID
#define DC_HASDEFID 0x534B
#endif

float monitorDpiScale(HMONITOR mon) {
    if (mon) {
        if (HMODULE shcore = LoadLibraryW(L"shcore.dll")) {
            using GetDpiForMonFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
            auto fn = (GetDpiForMonFn)(void*)GetProcAddress(shcore, "GetDpiForMonitor");
            UINT dx = 96, dy = 96;
            bool ok = (fn && SUCCEEDED(fn(mon, 0 /* MDT_EFFECTIVE_DPI */, &dx, &dy)) && dy > 0);
            FreeLibrary(shcore);
            if (ok) return (float)dy / 96.0f;
        }
    }
    HDC screen = GetDC(nullptr);
    float s = screen ? (GetDeviceCaps(screen, LOGPIXELSY) / 96.0f) : 1.0f;
    if (screen) ReleaseDC(nullptr, screen);
    return s > 0.0f ? s : 1.0f;
}

int measureTextWidth(HWND refWnd, HFONT font, const std::wstring& text) {
    if (text.empty()) return 0;
    HDC screen = GetDC(refWnd);
    HDC mem = CreateCompatibleDC(screen);
    HGDIOBJ oldF = mem ? SelectObject(mem, font) : nullptr;
    RECT rc = { 0, 0, 0, 0 };
    if (mem) {
        DrawTextW(mem, text.c_str(), (int)text.size(), &rc, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(mem, oldF);
        DeleteDC(mem);
    }
    if (screen) ReleaseDC(refWnd, screen);
    return (int)(rc.right - rc.left);
}

int measureWrappedTextHeight(HWND refWnd, HFONT font, const std::wstring& text, int widthPx) {
    if (text.empty()) return 0;
    HDC screen = GetDC(refWnd);
    HDC mem = CreateCompatibleDC(screen);
    HGDIOBJ oldF = mem ? SelectObject(mem, font) : nullptr;
    RECT rc = { 0, 0, std::max(40, widthPx), 0 };
    if (mem) {
        DrawTextW(mem, text.c_str(), (int)text.size(), &rc,
                  DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX);
        SelectObject(mem, oldF);
        DeleteDC(mem);
    }
    if (screen) ReleaseDC(refWnd, screen);
    return std::max(16, (int)(rc.bottom - rc.top));
}

void cleanupGdiResources(AppState& app) {
    if (app.updateFont) { DeleteObject(app.updateFont); app.updateFont = nullptr; }
    if (app.updateBoldFont) { DeleteObject(app.updateBoldFont); app.updateBoldFont = nullptr; }
    if (app.updateHeadingFont) { DeleteObject(app.updateHeadingFont); app.updateHeadingFont = nullptr; }
    if (app.updateBgBrush) { DeleteObject(app.updateBgBrush); app.updateBgBrush = nullptr; }
}
} // namespace update_ui

LRESULT CALLBACK updateWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* app = g_app;
    switch (msg) {
        case WM_ERASEBKGND:
            if (app && app->updateBgBrush) {
                RECT rc = {};
                GetClientRect(hwnd, &rc);
                FillRect((HDC)wParam, &rc, app->updateBgBrush);
                return TRUE;
            }
            break;
        case WM_CTLCOLORSTATIC:
            if (app && app->updateBgBrush) {
                HDC hdc = (HDC)wParam;
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, app->updateDark ? RGB(236, 238, 242) : RGB(28, 30, 34));
                return (LRESULT)app->updateBgBrush;
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps = {};
            HDC hdc = BeginPaint(hwnd, &ps);
            if (app) {
                HDC screen = GetDC(hwnd);
                float dpi = screen ? (GetDeviceCaps(screen, LOGPIXELSY) / 96.0f) : 1.0f;
                if (screen) ReleaseDC(hwnd, screen);
                int icoSz = std::max(24, (int)std::lround(36.0f * dpi));
                int icoX = std::max(14, (int)std::lround(20.0f * dpi));
                int icoY = std::max(14, (int)std::lround(20.0f * dpi));
                LPCWSTR iconId = app->updateIsWarning ? IDI_WARNING : IDI_INFORMATION;
                HICON hIco = (HICON)LoadImageW(nullptr, iconId, IMAGE_ICON, icoSz, icoSz, LR_SHARED);
                if (!hIco) hIco = LoadIconW(nullptr, iconId);
                if (hIco) {
                    DrawIconEx(hdc, icoX, icoY, hIco, icoSz, icoSz, 0, nullptr, DI_NORMAL);
                }
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case DM_GETDEFID:
            if (app) return MAKELRESULT((WORD)app->updateDefaultBtnId, DC_HASDEFID);
            break;
        case WM_COMMAND: {
            if (!app) return 0;
            int id = LOWORD(wParam);
            if (id == IDOK) {
                UpdateDialogKind kind = app->updateDialogKind;
                std::wstring url = app->updateDialogUrl.empty() ? UPDATE_PAGE_URL : app->updateDialogUrl;
                if (kind == UpdateDialogKind::UpdateAvailable) {
                    app->settings.suppressAutoUpdatePrompt = false;
                    app->settings.saveToRegistry();
                    DestroyWindow(hwnd);
                    startInAppUpdate(*app);
                } else if (kind == UpdateDialogKind::DownloadFailed) {
                    DestroyWindow(hwnd);
                    ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                } else {
                    DestroyWindow(hwnd);
                }
                return 0;
            }
            if (id == update_ui::ID_UPD_SECONDARY) {
                std::wstring url = app->updateDialogUrl.empty() ? UPDATE_PAGE_URL : app->updateDialogUrl;
                DestroyWindow(hwnd);
                ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            }
            if (id == IDCANCEL) {
                if (app->updateDialogKind == UpdateDialogKind::UpdateAvailable) {
                    app->settings.suppressAutoUpdatePrompt = true;
                    app->settings.saveToRegistry();
                }
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (app && app->updateWnd == hwnd) {
                app->updateWnd = nullptr;
                app->updateDialogKind = UpdateDialogKind::None;
                app->updateDialogUrl.clear();
                update_ui::cleanupGdiResources(*app);
                if (!app->replacingUpdateWnd) {
                    restoreTopmostAfterUpdateDialog(*app);
                }
            }
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void showUpdateResultWindow(AppState& app,
                            UpdateDialogKind kind,
                            const std::wstring& title,
                            const std::wstring& heading,
                            const std::wstring& body,
                            bool isWarning,
                            bool bringToFront,
                            const std::wstring& url,
                            const std::wstring& primaryLabel,
                            const std::wstring& secondaryLabel,
                            const std::wstring& cancelLabel) {
    if (app.updateWnd && IsWindow(app.updateWnd)) {
        app.replacingUpdateWnd = true;
        DestroyWindow(app.updateWnd);
        app.replacingUpdateWnd = false;
    }
    update_ui::cleanupGdiResources(app);

    HINSTANCE inst = GetModuleHandleW(nullptr);
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = updateWndProc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = UPDATE_WINDOW_CLASS;
        registered = RegisterClassExW(&wc) != 0;
    }

    POINT curPt = {};
    GetCursorPos(&curPt);
    HWND fg = GetForegroundWindow();
    HMONITOR mon = (fg && IsWindow(fg) && IsWindowVisible(fg) && fg != app.hwnd && fg != app.menuOwnerHwnd)
        ? MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST)
        : MonitorFromPoint(curPt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    RECT work = {};
    if (GetMonitorInfoW(mon, &mi)) {
        work = mi.rcWork;
    } else {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    }

    float dpiScale = update_ui::monitorDpiScale(mon);
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    RECT frameRc = { 0, 0, 0, 0 };
    AdjustWindowRectEx(&frameRc, style, FALSE, 0);
    int frameW = frameRc.right - frameRc.left, frameH = frameRc.bottom - frameRc.top;
    int maxClientW = std::max(320, (int)(work.right - work.left) - frameW - 24);
    int maxClientH = std::max(180, (int)(work.bottom - work.top) - frameH - 24);
    const int baseW = 472;
    float k = std::min(dpiScale, (float)maxClientW / (float)baseW);
    if (k < 0.65f) k = 0.65f;
    auto px = [&](int v) { return (int)std::lround(v * k); };

    app.updateDark = settings_ui::isWindowsDarkMode();
    app.updateIsWarning = isWarning;
    app.updateDialogKind = kind;
    app.updateDialogUrl = url;
    app.updateDefaultBtnId = IDOK;
    app.updateBgBrush = CreateSolidBrush(app.updateDark ? RGB(32, 32, 36) : RGB(246, 246, 248));
    app.updateFont = CreateFontW(-px(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    app.updateBoldFont = CreateFontW(-px(13), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    app.updateHeadingFont = CreateFontW(-px(15), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    int clientW = px(baseW);
    int textX = px(72);
    int textW = std::max(120, clientW - textX - px(22));
    int headH = update_ui::measureWrappedTextHeight(nullptr, app.updateHeadingFont, heading, textW) + px(2);
    int bodyH = update_ui::measureWrappedTextHeight(nullptr, app.updateFont, body, textW) + px(6);
    int btnH = px(28);
    int btnY = std::max(px(72), px(20) + headH + px(8) + bodyH + px(18));
    int clientH = std::min(maxClientH, btnY + btnH + px(18));
    if (clientH < btnY + btnH + px(12)) {
        btnY = std::max(px(40), clientH - btnH - px(14));
        bodyH = std::max(px(24), btnY - (px(20) + headH + px(8) + px(10)));
    }

    RECT rc = { 0, 0, clientW, clientH };
    AdjustWindowRectEx(&rc, style, FALSE, 0);
    int winW = rc.right - rc.left, winH = rc.bottom - rc.top;
    int posX = std::max((int)work.left, (int)(work.left + work.right - winW) / 2);
    int posY = std::max((int)work.top, (int)(work.top + work.bottom - winH) / 2);

    // Yield topmost on the cat / dressing room before showing the non-modal dialog so the
    // dialog is never obscured by the overlay window while remaining non-topmost itself.
    demoteTopmostForUpdateDialog(app);

    HWND win = CreateWindowExW(0, UPDATE_WINDOW_CLASS, title.c_str(), style,
        posX, posY, winW, winH, nullptr, nullptr, inst, nullptr);
    if (!win) {
        update_ui::cleanupGdiResources(app);
        app.updateDialogKind = UpdateDialogKind::None;
        app.updateDialogUrl.clear();
        restoreTopmostAfterUpdateDialog(app);
        return;
    }
    app.updateWnd = win;
    settings_ui::applyDwmDarkTitleBar(win, app.updateDark);

    HWND hHead = CreateWindowExW(0, L"STATIC", heading.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_EDITCONTROL | SS_NOPREFIX,
        textX, px(20), textW, headH, win, (HMENU)(INT_PTR)update_ui::ID_UPD_HEADING, inst, nullptr);
    if (hHead) SendMessageW(hHead, WM_SETFONT, (WPARAM)app.updateHeadingFont, TRUE);

    HWND hBody = CreateWindowExW(0, L"STATIC", body.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_EDITCONTROL | SS_NOPREFIX,
        textX, px(20) + headH + px(8), textW, bodyH, win, (HMENU)(INT_PTR)update_ui::ID_UPD_BODY, inst, nullptr);
    if (hBody) SendMessageW(hBody, WM_SETFONT, (WPARAM)app.updateFont, TRUE);

    int rightEdge = clientW - px(20);
    int btnGap = px(8);
    auto addBtn = [&](const std::wstring& label, int id, bool isDefault) {
        if (label.empty()) return;
        HFONT fnt = isDefault ? app.updateBoldFont : app.updateFont;
        int measuredW = update_ui::measureTextWidth(win, fnt, label);
        int bw = std::clamp(measuredW + px(28), px(84), px(176));
        rightEdge -= bw;
        HWND b = CreateWindowExW(0, L"BUTTON", label.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | (isDefault ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
            rightEdge, btnY, bw, btnH, win, (HMENU)(INT_PTR)id, inst, nullptr);
        if (b) SendMessageW(b, WM_SETFONT, (WPARAM)fnt, TRUE);
        rightEdge -= btnGap;
    };

    // Add right-to-left: Cancel -> Secondary -> Primary
    addBtn(cancelLabel, IDCANCEL, false);
    addBtn(secondaryLabel, update_ui::ID_UPD_SECONDARY, false);
    addBtn(primaryLabel, IDOK, true);

    bool fullscreenFg = isForegroundFullscreen(app.hwnd);
    bool shouldSurface = bringToFront || !fullscreenFg;
    ShowWindow(win, SW_SHOWNOACTIVATE);
    if (shouldSurface) {
        // Briefly raise via HWND_TOPMOST -> HWND_NOTOPMOST so the dialog appears visibly
        // above normal windows the user switched to, without staying permanently topmost.
        SetWindowPos(win, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        SetWindowPos(win, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    } else {
        SetWindowPos(win, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
    if (app.hwnd && IsWindow(app.hwnd) && app.isVisible && !app.hiddenByFullscreen &&
        !(app.settings.obsCaptureMode > 0 && app.settings.obsHideDesktop)) {
        SetWindowPos(app.hwnd, win, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
    if (shouldSurface) {
        AllowSetForegroundWindow(ASFW_ANY);
        BringWindowToTop(win);
        SetForegroundWindow(win);
        if (GetForegroundWindow() == win) {
            if (HWND defBtn = GetDlgItem(win, IDOK)) SetFocus(defBtn);
        }
    }
}

void handleUpdateCheckResult(AppState& app, const UpdateCheckResult& res) {
    // Picks the zh / en / pt variant for the current UI language.
    const int lang = app.settings.language;
    auto pick = [lang](std::wstring zh, std::wstring en, std::wstring pt) { return lang == 1 ? en : lang == 2 ? pt : zh; };
    if (res.ok && res.hasUpdate) {
        app.availableUpdateVersion = res.latestTag;
        app.availableUpdateUrl = res.releaseUrl;
        if (!res.manual && app.settings.suppressAutoUpdatePrompt) return;

        std::wstring title = pick(L"KuroCat 更新提醒", L"KuroCat Update Available", L"Atualização do KuroCat");
        std::wstring heading = pick(L"发现 KuroCat 新版本 " + res.latestTag,
                                    L"New KuroCat version " + res.latestTag + L" is available",
                                    L"Nova versão do KuroCat: " + res.latestTag);
        std::wstring body = pick(
            L"当前版本：v" + std::wstring(APP_VERSION)
                + L"\n当前运行的程序：" + currentExePath()
                + L"\n\n如果你已经下载过新版，直接双击新版 exe 即可自动替换掉这个旧版。"
                + L"\n\n点击「立即更新」将自动下载并更新重启；\n点击「暂不更新」后续将不再自动弹窗提醒（随时可在右键菜单点击「检查更新」）。",
            L"Current version: v" + std::wstring(APP_VERSION)
                + L"\nRunning executable: " + currentExePath()
                + L"\n\nIf you already downloaded the new version, double-click the new exe to replace this copy automatically."
                + L"\n\nClick \"Update Now\" to download, update, and restart automatically.\nClick \"Later\" to stop automatic update prompts (you can still check manually from the menu).",
            L"Versão atual: v" + std::wstring(APP_VERSION)
                + L"\nPrograma em execução: " + currentExePath()
                + L"\n\nSe você já baixou a nova versão, dê dois cliques no novo .exe para substituir esta cópia automaticamente."
                + L"\n\nClique em \"Atualizar agora\" para baixar, atualizar e reiniciar automaticamente.\nClique em \"Depois\" para parar os avisos automáticos (você ainda pode verificar pelo menu).");
        showUpdateResultWindow(
            app, UpdateDialogKind::UpdateAvailable, title, heading, body, false, res.manual,
            res.releaseUrl,
            pick(L"立即更新", L"Update Now", L"Atualizar agora"),
            pick(L"打开下载页", L"Download Page", L"Página de download"),
            pick(L"暂不更新", L"Later", L"Depois"));
    } else if (res.manual) {
        std::wstring title = pick(L"KuroCat 检查更新", L"KuroCat Check for Updates", L"KuroCat – Verificar atualizações");
        if (res.ok) {
            std::wstring heading = pick(L"已是最新版本", L"You are up to date", L"Você está atualizado");
            std::wstring body = pick(
                L"当前版本 v" + std::wstring(APP_VERSION) + L" 已是最新版本。",
                L"Current version v" + std::wstring(APP_VERSION) + L" is already the latest version.",
                L"A versão atual v" + std::wstring(APP_VERSION) + L" já é a mais recente.");
            showUpdateResultWindow(
                app, UpdateDialogKind::UpToDate, title, heading, body, false, true,
                UPDATE_PAGE_URL,
                pick(L"好", L"OK", L"OK"), L"", L"");
        } else {
            std::wstring heading = pick(L"无法连接到更新服务器", L"Unable to check for updates", L"Não foi possível verificar atualizações");
            std::wstring body = pick(
                L"无法连接到更新服务器，请稍后重试或检查网络。",
                L"Could not reach the update server. Please check your network connection and try again later.",
                L"Não foi possível conectar ao servidor de atualizações. Verifique sua conexão e tente de novo mais tarde.");
            showUpdateResultWindow(
                app, UpdateDialogKind::CheckFailed, title, heading, body, true, true,
                UPDATE_PAGE_URL,
                pick(L"好", L"OK", L"OK"),
                pick(L"打开下载页", L"Download Page", L"Página de download"),
                L"");
        }
    }
}

void handleUpdateDownloadResult(AppState& app, const UpdateDownloadResult& res) {
    bool launched = false;
    if (res.ok && !res.newExePath.empty() && !res.targetExePath.empty()) {
        std::wstring args = L"--apply-update " + std::to_wstring(GetCurrentProcessId())
                          + L" \"" + res.targetExePath + L"\"";
        launched = launchProcessArgs(res.newExePath, args);
    }
    if (launched) {
        PostQuitMessage(0);
        return;
    }
    const int lang = app.settings.language;
    auto pick = [lang](std::wstring zh, std::wstring en, std::wstring pt) { return lang == 1 ? en : lang == 2 ? pt : zh; };
    std::wstring url = !app.availableUpdateUrl.empty() ? app.availableUpdateUrl : UPDATE_PAGE_URL;
    std::wstring title = pick(L"KuroCat 自动更新", L"KuroCat Auto Update", L"KuroCat – Atualização automática");
    std::wstring heading = pick(L"自动下载或替换安装包失败", L"Automatic update failed", L"Falha na atualização automática");
    std::wstring body = pick(
        L"自动下载或替换安装包失败，是否打开下载页手动下载？",
        L"Failed to download or replace the executable. Would you like to open the download page to download manually?",
        L"Falha ao baixar ou substituir o executável. Quer abrir a página de download para baixar manualmente?");
    showUpdateResultWindow(
        app, UpdateDialogKind::DownloadFailed, title, heading, body, true, true,
        url,
        pick(L"打开下载页", L"Open Download Page", L"Abrir página de download"),
        L"",
        pick(L"取消", L"Cancel", L"Cancelar"));
}

void showContextMenu(AppState& app, POINT screenPt) {
    HMENU menu = CreatePopupMenu();
    auto tr = [&](const wchar_t* zh, const wchar_t* eng, const wchar_t* pt) -> const wchar_t* { return app.settings.tr(zh, eng, pt); };

    // Status & live stats header
    std::wstring statusStr = app.input.isRawInputRegistered
        ? tr(L"\u25CF \u786C\u4EF6\u7EA7\u5168\u72B6\u6001\u76D1\u542C\u4E2D (Raw Input)",
             L"\u25CF Hardware Input Active (Raw Input)", L"● Captura de hardware ativa (Raw Input)")
        : tr(L"\u26A0 Raw Input \u672A\u542F\u52A8", L"\u26A0 Raw Input Inactive", L"⚠ Raw Input inativo");
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, statusStr.c_str());
    if (!app.isElevated && app.lastBlockedForegroundTime > 0.0 && nowSeconds() - app.lastBlockedForegroundTime < 600.0) {
        AppendMenuW(menu, MF_STRING, CMD_RELAUNCH_ADMIN,
            tr(L"\u26A0 \u68C0\u6D4B\u5230\u7BA1\u7406\u5458\u7A97\u53E3\u62E6\u622A\u952E\u76D8\uFF0C\u70B9\u6B64\u7BA1\u7406\u5458\u91CD\u542F",
               L"\u26A0 Admin app blocked keys — Restart as Administrator", L"⚠ App de administrador bloqueou as teclas — reiniciar como administrador"));
    }

    const int lang = app.settings.language;
    std::wstring stats = lang == 1
        ? (L"Today " + std::to_wstring(app.input.totalKeystrokesToday)
            + L"  \u00B7  KPM " + std::to_wstring(app.input.currentKPM())
            + L"  \u00B7  Pets " + std::to_wstring(app.physics.totalPetSessions))
        : lang == 2
        ? (L"Hoje " + std::to_wstring(app.input.totalKeystrokesToday)
            + L"  \u00B7  KPM " + std::to_wstring(app.input.currentKPM())
            + L"  \u00B7  Carinhos " + std::to_wstring(app.physics.totalPetSessions))
        : (L"\u4ECA\u65E5\u51FB\u952E " + std::to_wstring(app.input.totalKeystrokesToday)
            + L"  \u00B7  KPM " + std::to_wstring(app.input.currentKPM())
            + L"  \u00B7  \u6478\u732B " + std::to_wstring(app.physics.totalPetSessions) + L" \u6B21");
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, stats.c_str());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    // Essential quick actions
    AppendMenuW(menu, MF_STRING, CMD_TOGGLE_VISIBLE, app.isVisible ? tr(L"\u9690\u85CF\u684C\u5BA0", L"Hide Pet", L"Ocultar gatinho") : tr(L"\u663E\u793A\u684C\u5BA0", L"Show Pet", L"Mostrar gatinho"));

    // Quick Cat Skin switcher
    HMENU coatMenu = CreatePopupMenu();
    bool customActive = app.skins.find(app.settings.customSkin) != nullptr;
    auto coatCheck = [&](CatCoatTheme c) -> UINT { return !customActive && app.settings.coatTheme == c ? MF_CHECKED : 0; };
    AppendMenuW(coatMenu, MF_STRING | coatCheck(CatCoatTheme::ClassicBlack), CMD_COAT_BLACK, tr(L"\u7ECF\u5178\u5C0F\u9ED1\u732B (\u539F\u7248)", L"Classic Black Cat", L"Gatinho preto clássico"));
    AppendMenuW(coatMenu, MF_STRING | coatCheck(CatCoatTheme::Siamese), CMD_COAT_SIAMESE, tr(L"\u6316\u7164\u66B9\u7F57\u732B", L"Siamese Cat", L"Gato siamês"));
    AppendMenuW(coatMenu, MF_STRING | coatCheck(CatCoatTheme::Shorthair), CMD_COAT_SHORTHAIR, tr(L"\u7F8E\u77ED\uFF08\u94F6\u864E\u6591\u52A0\u767D\uFF09", L"Silver Tabby Shorthair", L"Pelo curto tigrado prateado"));
    AppendMenuW(coatMenu, MF_STRING | coatCheck(CatCoatTheme::OrangeWhite), CMD_COAT_ORANGE_WHITE, tr(L"\u6A58\u767D\u732B", L"Orange & White Cat", L"Gato laranja e branco"));
    AppendMenuW(coatMenu, MF_STRING | coatCheck(CatCoatTheme::Dazed), CMD_COAT_DAZED, tr(L"\u5446\u732B", L"Dazed Cat", L"Gato atordoado"));
    AppendMenuW(coatMenu, MF_STRING | coatCheck(CatCoatTheme::Tabby), CMD_COAT_TABBY, tr(L"\u72F8\u82B1\u732B", L"Tabby Cat", L"Gato tigrado"));
    size_t listed = std::min<size_t>(app.skins.skins.size(), CMD_SKIN_RANGE);
    if (listed) AppendMenuW(coatMenu, MF_SEPARATOR, 0, nullptr);
    for (size_t i = 0; i < listed; ++i) {
        const SkinSpec& s = app.skins.skins[i];
        AppendMenuW(coatMenu, MF_STRING | (app.settings.customSkin == s.id ? MF_CHECKED : 0),
                    CMD_SKIN_SELECT_BASE + (UINT)i, utf8ToWide(app.skins.displayName(s)).c_str());
    }
    if (listed) {
        AppendMenuW(coatMenu, MF_SEPARATOR, 0, nullptr);
        HMENU renameMenu = CreatePopupMenu();
        HMENU deleteMenu = CreatePopupMenu();
        for (size_t i = 0; i < listed; ++i) {
            std::wstring label = utf8ToWide(app.skins.displayName(app.skins.skins[i]));
            AppendMenuW(renameMenu, MF_STRING, CMD_SKIN_RENAME_BASE + (UINT)i, (L"\u270F " + label).c_str());
            AppendMenuW(deleteMenu, MF_STRING, CMD_SKIN_DELETE_BASE + (UINT)i, (L"\U0001F5D1 " + label).c_str());
        }
        AppendMenuW(coatMenu, MF_POPUP, (UINT_PTR)renameMenu, tr(L"\u270F \u91CD\u547D\u540D\u76AE\u80A4\u2026", L"\u270F Rename Skin...", L"✏ Renomear skin…"));
        AppendMenuW(coatMenu, MF_POPUP, (UINT_PTR)deleteMenu, tr(L"\U0001F5D1 \u5220\u9664\u5DF2\u5BFC\u5165\u76AE\u80A4\u2026", L"\U0001F5D1 Delete Imported Skin...", L"🗑 Excluir skin importada…"));
        AppendMenuW(coatMenu, MF_STRING, CMD_BATCH_DELETE_SKINS, tr(L"\U0001F4E6 \u6279\u91CF\u5220\u9664\u76AE\u80A4\u2026", L"\U0001F4E6 Batch Delete Skins...", L"📦 Excluir várias skins…"));
    }
    AppendMenuW(coatMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(coatMenu, MF_STRING, CMD_IMPORT_SKIN, tr(L"\u5BFC\u5165\u76AE\u80A4 / \u6362\u88C5\u95F4\u2026", L"Import Skin / Wardrobe...", L"Importar skin / Provador…"));
    AppendMenuW(coatMenu, MF_STRING, CMD_OPEN_WORKSHOP, tr(L"\u6253\u5F00\u76AE\u80A4\u5DE5\u574A\u2026", L"Open Skin Workshop...", L"Abrir oficina de skins…"));
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)coatMenu, tr(L"\u732B\u54AA\u76AE\u80A4 \U0001F431", L"Cat Skin \U0001F431", L"Skin do gato 🐱"));

    // Skin Workshop (Create custom skins) right on the top-level menu
    AppendMenuW(menu, MF_STRING, CMD_OPEN_WORKSHOP,
        tr(L"\U0001F3A8 \u76AE\u80A4\u5DE5\u574A\uFF08\u5236\u4F5C\u76AE\u80A4\uFF09\u2026",
           L"\U0001F3A8 Skin Workshop (Create Skins)...", L"🎨 Oficina de skins (criar skins)…"));

    AppendMenuW(menu, MF_STRING, CMD_OPEN_WEBSITE,
        tr(L"\U0001F310 \u8F6F\u4EF6\u5B98\u7F51 (kurocat.cc)",
           L"\U0001F310 Website (kurocat.cc)", L"🌐 Site oficial (kurocat.cc)"));

    AppendMenuW(menu, MF_STRING | (app.settings.levelKeyboard ? MF_CHECKED : 0), CMD_TOGGLE_LEVEL_KEYBOARD,
        tr(L"\u952E\u76D8\u6C34\u5E73\u6446\u653E", L"Level Keyboard", L"Teclado nivelado (paralelo à tela)"));

    // Language switcher (labels stay readable whatever the current language is)
    HMENU langMenu = CreatePopupMenu();
    AppendMenuW(langMenu, MF_STRING | (app.settings.language == 0 ? MF_CHECKED : 0), CMD_LANG_ZH, L"\u7B80\u4F53\u4E2D\u6587");
    AppendMenuW(langMenu, MF_STRING | (app.settings.language == 1 ? MF_CHECKED : 0), CMD_LANG_EN, L"English");
    AppendMenuW(langMenu, MF_STRING | (app.settings.language == 2 ? MF_CHECKED : 0), CMD_LANG_PT, L"Português (Brasil)");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)langMenu, L"\u8BED\u8A00 / Language / Idioma");

    // Unified Settings window
    AppendMenuW(menu, MF_STRING, CMD_OPEN_SETTINGS, tr(L"\u2699\uFE0F \u8BBE\u7F6E\u2026", L"\u2699\uFE0F Settings...", L"⚙️ Configurações…"));

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    if (!app.availableUpdateVersion.empty()) {
        std::wstring upTxt = app.isUpdating
            ? (std::wstring(tr(L"\u23F3 \u6B63\u5728\u4E0B\u8F7D\u66F4\u65B0 ", L"\u23F3 Downloading update ", L"⏳ Baixando atualização ")) + app.availableUpdateVersion + L"...")
            : (std::wstring(tr(L"\U0001F195 \u53D1\u73B0\u65B0\u7248\u672C ", L"\U0001F195 Update Available ", L"🆕 Nova versão ")) + app.availableUpdateVersion);
        AppendMenuW(menu, MF_STRING | (app.isUpdating ? MF_GRAYED : 0), CMD_OPEN_UPDATE, upTxt.c_str());
    } else if (app.isManualUpdateCheck) {
        AppendMenuW(menu, MF_STRING | MF_GRAYED, CMD_CHECK_UPDATE,
            tr(L"\u23F3 \u6B63\u5728\u68C0\u67E5\u66F4\u65B0...", L"\u23F3 Checking for Updates...", L"\u23F3 Verificando atualizações..."));
    } else {
        std::wstring chkTxt = std::wstring(tr(L"\u68C0\u67E5\u66F4\u65B0 (\u5F53\u524D v", L"Check for Updates (v", L"Verificar atualizações (v")) + APP_VERSION + L")";
        AppendMenuW(menu, MF_STRING, CMD_CHECK_UPDATE, chkTxt.c_str());
    }
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, CMD_QUIT, tr(L"\u9000\u51FA KuroCat", L"Quit KuroCat", L"Sair do KuroCat"));

    HWND menuOwner = app.menuOwnerHwnd ? app.menuOwnerHwnd : app.hwnd;
    app.inContextMenu = true;
    if (app.menuOwnerHwnd) {
        SetWindowPos(menuOwner, HWND_TOPMOST, screenPt.x, screenPt.y, 1, 1, SWP_NOACTIVATE);
        ShowWindow(menuOwner, SW_SHOWNA);
    }
    HWND fg = GetForegroundWindow();
    DWORD curTid = GetCurrentThreadId();
    DWORD fgTid = (fg && fg != menuOwner) ? GetWindowThreadProcessId(fg, nullptr) : 0;
    bool attached = (fgTid && fgTid != curTid) ? (AttachThreadInput(curTid, fgTid, TRUE) != FALSE) : false;
    AllowSetForegroundWindow(ASFW_ANY);
    BringWindowToTop(menuOwner);
    SetForegroundWindow(menuOwner);
    if (attached) AttachThreadInput(curTid, fgTid, FALSE);
    UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screenPt.x, screenPt.y, 0, menuOwner, nullptr);
    PostMessageW(menuOwner, WM_NULL, 0, 0);
    if (app.menuOwnerHwnd) {
        ShowWindow(menuOwner, SW_HIDE);
    }
    app.inContextMenu = false;
    DestroyMenu(menu);

    switch (cmd) {
        case CMD_TOGGLE_VISIBLE:
            app.isVisible = !app.isVisible;
            app.hiddenByFullscreen = false;
            ShowWindow(app.hwnd, app.isVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
            break;
        case CMD_TOGGLE_CLICKTHROUGH:
            app.settings.fullClickThrough = !app.settings.fullClickThrough;
            app.settings.saveToRegistry();
            if (app.settings.fullClickThrough) {
                showTrayBalloon(app,
                    app.settings.tr(L"KuroCat \u5DF2\u5F00\u542F\u5B8C\u5168\u9F20\u6807\u7A7F\u900F",
                                    L"KuroCat full click-through is on",
                                    L"KuroCat: ignorar cliques ativado"),
                    app.settings.tr(L"\u53D6\u6D88\u65B9\u5F0F\uFF1A\u6309 Ctrl+Alt+Shift+P\uFF0C\u6216\u6309\u4F4F Alt \u952E\u53F3\u952E\u5C0F\u732B\uFF0C\u6216\u70B9\u53F3\u4E0B\u89D2\u6258\u76D8\u5C0F\u732B\u56FE\u6807\uFF0C\u6216\u518D\u6B21\u53CC\u51FB KuroCat.exe",
                                    L"To turn it off: press Ctrl+Alt+Shift+P, hold Alt and right-click the cat, click the cat icon in the tray, or run KuroCat.exe again.",
                                    L"Para desativar: pressione Ctrl+Alt+Shift+P, segure Alt e clique com o botão direito no gato, clique no ícone do gato na bandeja ou abra o KuroCat.exe de novo."));
            }
            break;
        case CMD_TOGGLE_TOPMOST:
            app.settings.alwaysOnTop = !app.settings.alwaysOnTop;
            applyWindowScaleAndPosition(app, false);
            break;
        case CMD_TOGGLE_AUTOHIDE_FS:
            app.settings.autoHideFullscreen = !app.settings.autoHideFullscreen;
            app.settings.saveToRegistry();
            if (!app.settings.autoHideFullscreen && app.hiddenByFullscreen) {
                app.hiddenByFullscreen = false;
                if (app.isVisible) {
                    ShowWindow(app.hwnd, SW_SHOWNOACTIVATE);
                    assertTopmost(app);
                }
            }
            app.nextHousekeepingTime = 0.0;
            break;
        case CMD_TOGGLE_AUTOSTART:
            setAutoStartEnabled(!isAutoStartEnabled());
            break;
        case CMD_TOGGLE_BADGES:
            app.settings.showKeyBadges = !app.settings.showKeyBadges;
            app.settings.saveToRegistry();
            break;
        case CMD_TOGGLE_MOUSE_BADGES:
            app.settings.showMouseBadges = !app.settings.showMouseBadges;
            app.settings.saveToRegistry();
            break;
        case CMD_TOGGLE_KEY_LEGENDS:
            app.settings.showKeyLegends = !app.settings.showKeyLegends;
            app.settings.saveToRegistry();
            break;
        case CMD_TOGGLE_FOOT:
            app.settings.footEasterEgg = !app.settings.footEasterEgg;
            app.settings.saveToRegistry();
            break;
        case CMD_TOGGLE_TAIL:
            app.settings.tailEasterEgg = !app.settings.tailEasterEgg;
            app.settings.saveToRegistry();
            break;
        case CMD_TOGGLE_MARQUEE:
            app.settings.keyboardMarquee = !app.settings.keyboardMarquee;
            app.settings.saveToRegistry();
            break;
        case CMD_TOGGLE_LEVEL_KEYBOARD:
            app.settings.levelKeyboard = !app.settings.levelKeyboard;
            KeyboardLayout::levelKeyboard() = app.settings.levelKeyboard;
            app.settings.saveToRegistry();
            settings_ui::rebuildCombosAndSync(app);
            break;
        case CMD_TOGGLE_PET:
            app.settings.petInteraction = !app.settings.petInteraction;
            app.settings.saveToRegistry();
            break;
        case CMD_OBS_OFF:
            app.settings.obsCaptureMode = 0;
            app.settings.obsHideDesktop = false;
            app.settings.saveToRegistry();
            applyOBSMode(app);
            break;
        case CMD_OBS_TRANSPARENT:
            app.settings.obsCaptureMode = 1;
            app.settings.saveToRegistry();
            applyOBSMode(app);
            break;
        case CMD_OBS_GREENSCREEN:
            app.settings.obsCaptureMode = 2;
            app.settings.saveToRegistry();
            applyOBSMode(app);
            break;
        case CMD_TOGGLE_OBS_HIDE:
            app.settings.obsHideDesktop = !app.settings.obsHideDesktop;
            app.settings.saveToRegistry();
            applyOBSMode(app);
            break;
        case CMD_TOGGLE_EDIT:
            app.settings.isEditMode = !app.settings.isEditMode;
            break;
        case CMD_LANG_ZH:
            applyUiLanguage(app, 0);
            break;
        case CMD_LANG_EN:
            applyUiLanguage(app, 1);
            break;
        case CMD_LANG_PT:
            applyUiLanguage(app, 2);
            break;
        case CMD_TABLET_AUTO:
            app.settings.tabletMode = 0;
            app.settings.saveToRegistry();
            break;
        case CMD_TABLET_ON:
            app.settings.tabletMode = 1;
            app.settings.saveToRegistry();
            break;
        case CMD_TABLET_OFF:
            app.settings.tabletMode = 2;
            app.settings.saveToRegistry();
            break;
        case CMD_IMPORT_SKIN:
            showDressingRoom(app);
            break;
        case CMD_OPEN_WORKSHOP:
            ShellExecuteW(nullptr, L"open", L"https://chanke17.github.io/KuroCat-Workshop/", nullptr, nullptr, SW_SHOWNORMAL);
            break;
        case CMD_OPEN_WEBSITE:
            ShellExecuteW(nullptr, L"open", L"https://kurocat.cc", nullptr, nullptr, SW_SHOWNORMAL);
            break;
        case CMD_OPEN_SKINS_DIR:
            SkinLibrary::ensureDir();
            ShellExecuteW(nullptr, L"open", SkinLibrary::dir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            break;
        case CMD_IMPORT_KB:
            chooseKeyboardSkinFile(app);
            break;
        case CMD_OPEN_KB_PLAZA:
            ShellExecuteW(nullptr, L"open", L"https://chanke17.github.io/KuroCat-Workshop/#keyboards", nullptr, nullptr, SW_SHOWNORMAL);
            break;
        case CMD_OPEN_KB_DIR:
            KeyboardSkinLibrary::ensureDir();
            ShellExecuteW(nullptr, L"open", KeyboardSkinLibrary::directory().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            break;
        case CMD_COAT_BLACK:
            app.settings.overrideSkinIris = false;
            app.settings.customSkin.clear();
            app.settings.coatTheme = CatCoatTheme::ClassicBlack;
            app.settings.eyeTheme = EyeColorTheme::NeonChartreuse;
            app.settings.saveToRegistry();
            break;
        case CMD_COAT_SIAMESE:
            app.settings.overrideSkinIris = false;
            app.settings.customSkin.clear();
            app.settings.coatTheme = CatCoatTheme::Siamese;
            app.settings.eyeTheme = EyeColorTheme::SiameseBlue;
            app.settings.saveToRegistry();
            break;
        case CMD_COAT_TABBY:
            app.settings.overrideSkinIris = false;
            app.settings.customSkin.clear();
            app.settings.coatTheme = CatCoatTheme::Tabby;
            app.settings.eyeTheme = EyeColorTheme::OliveGold;
            app.settings.saveToRegistry();
            break;
        case CMD_COAT_SHORTHAIR:
        case CMD_COAT_ORANGE_WHITE:
            app.settings.overrideSkinIris = false;
            app.settings.customSkin.clear();
            app.settings.coatTheme = cmd == CMD_COAT_SHORTHAIR ? CatCoatTheme::Shorthair : CatCoatTheme::OrangeWhite;
            app.settings.eyeTheme = cmd == CMD_COAT_SHORTHAIR ? EyeColorTheme::ReferenceHazel : EyeColorTheme::WarmAmber;
            app.settings.saveToRegistry();
            break;
        case CMD_COAT_DAZED:
            app.settings.overrideSkinIris = false;
            app.settings.customSkin.clear();
            app.settings.coatTheme = CatCoatTheme::Dazed;
            app.settings.eyeTheme = EyeColorTheme::SoftAmber;
            app.settings.saveToRegistry();
            break;
        case CMD_EYE_SOFT_AMBER: app.settings.overrideSkinIris = true; app.settings.eyeTheme = EyeColorTheme::SoftAmber; app.settings.saveToRegistry(); break;
        case CMD_EYE_REFERENCE_HAZEL: app.settings.overrideSkinIris = true; app.settings.eyeTheme = EyeColorTheme::ReferenceHazel; app.settings.saveToRegistry(); break;
        case CMD_EYE_OLIVE_GOLD: app.settings.overrideSkinIris = true; app.settings.eyeTheme = EyeColorTheme::OliveGold; app.settings.saveToRegistry(); break;
        case CMD_KB_MILK:
            app.settings.customKeyboard.clear();
            app.settings.keyboardTheme = KeyboardTheme::ClassicMilk;
            app.settings.saveToRegistry();
            break;
        case CMD_KB_HACKER:
            app.settings.customKeyboard.clear();
            app.settings.keyboardTheme = KeyboardTheme::MidnightHacker;
            app.settings.saveToRegistry();
            break;
        case CMD_KB_RETRO:
            app.settings.customKeyboard.clear();
            app.settings.keyboardTheme = KeyboardTheme::RetroCream;
            app.settings.saveToRegistry();
            break;
        case CMD_EYE_LIME:  app.settings.overrideSkinIris = true; app.settings.eyeTheme = EyeColorTheme::NeonChartreuse; app.settings.saveToRegistry(); break;
        case CMD_EYE_BLUE:  app.settings.overrideSkinIris = true; app.settings.eyeTheme = EyeColorTheme::SiameseBlue; app.settings.saveToRegistry(); break;
        case CMD_EYE_AMBER: app.settings.overrideSkinIris = true; app.settings.eyeTheme = EyeColorTheme::WarmAmber; app.settings.saveToRegistry(); break;
        case CMD_EYE_CYAN:  app.settings.overrideSkinIris = true; app.settings.eyeTheme = EyeColorTheme::IceCyan; app.settings.saveToRegistry(); break;
        case CMD_SCALE_65:  app.settings.scale = 0.65f; applyWindowScaleAndPosition(app, true); break;
        case CMD_SCALE_85:  app.settings.scale = 0.85f; applyWindowScaleAndPosition(app, true); break;
        case CMD_SCALE_100: app.settings.scale = 1.00f; applyWindowScaleAndPosition(app, true); break;
        case CMD_SCALE_125: app.settings.scale = 1.25f; applyWindowScaleAndPosition(app, true); break;
        case CMD_SCALE_150: app.settings.scale = 1.50f; applyWindowScaleAndPosition(app, true); break;
        case CMD_DEMO_BURST:
            app.demoActive = true;
            app.demoStartTime = nowSeconds();
            app.demoStepIdx = 0;
            break;
        case CMD_RESET_POS:
            app.settings.posX = -99999;
            app.settings.posY = -99999;
            applyWindowScaleAndPosition(app, false);
            break;
        case CMD_CHECK_UPDATE:
            // PT-BR fork: the in-app updater installs unverified binaries from a third-party mirror,
            // so "Check for Updates" just opens this fork's Releases page.
            ShellExecuteW(nullptr, L"open", FORK_RELEASES_URL, nullptr, nullptr, SW_SHOWNORMAL);
            break;
        case CMD_OPEN_UPDATE: {
            startInAppUpdate(app);
            break;
        }
        case CMD_RELAUNCH_ADMIN:
            // The elevated copy starts with --takeover and waits for this one to exit.
            if (relaunchSelfElevated(L"--takeover --no-elevate")) quitApp(app);
            break;
        case CMD_TOGGLE_RUN_AS_ADMIN:
            app.settings.runAsAdmin = !app.settings.runAsAdmin;
            app.settings.saveToRegistry();
            if (app.settings.runAsAdmin && !app.isElevated && relaunchSelfElevated(L"--takeover --no-elevate")) {
                quitApp(app);
            }
            break;
        case CMD_OPEN_SETTINGS:
            showSettingsWindow(app);
            break;
        case CMD_BATCH_DELETE_SKINS:
            confirmBatchDeleteSkins(app);
            break;
        case CMD_QUIT:
            quitApp(app);
            break;
        default:
            if (cmd >= CMD_SKIN_SELECT_BASE && cmd < CMD_SKIN_SELECT_BASE + CMD_SKIN_RANGE && cmd - CMD_SKIN_SELECT_BASE < app.skins.skins.size()) {
                app.settings.overrideSkinIris = false;
                app.settings.customSkin = app.skins.skins[cmd - CMD_SKIN_SELECT_BASE].id;
                app.settings.saveToRegistry();
            } else if (cmd >= CMD_SKIN_DELETE_BASE && cmd < CMD_SKIN_DELETE_BASE + CMD_SKIN_RANGE && cmd - CMD_SKIN_DELETE_BASE < app.skins.skins.size()) {
                confirmDeleteSkin(app, app.skins.skins[cmd - CMD_SKIN_DELETE_BASE].id);
            } else if (cmd >= CMD_SKIN_RENAME_BASE && cmd < CMD_SKIN_RENAME_BASE + CMD_SKIN_RANGE && cmd - CMD_SKIN_RENAME_BASE < app.skins.skins.size()) {
                promptRenameSkin(app, app.skins.skins[cmd - CMD_SKIN_RENAME_BASE].id);
            } else if (cmd >= CMD_KB_SELECT_BASE && cmd < CMD_KB_SELECT_BASE + CMD_SKIN_RANGE && cmd - CMD_KB_SELECT_BASE < app.kbLibrary.skins.size()) {
                app.settings.customKeyboard = app.kbLibrary.skins[cmd - CMD_KB_SELECT_BASE].id;
                app.settings.saveToRegistry();
            } else if (cmd >= CMD_KB_DELETE_BASE && cmd < CMD_KB_DELETE_BASE + CMD_SKIN_RANGE && cmd - CMD_KB_DELETE_BASE < app.kbLibrary.skins.size()) {
                confirmDeleteKeyboardSkin(app, app.kbLibrary.skins[cmd - CMD_KB_DELETE_BASE].id);
            }
            break;
    }
    refreshDressingRoom(app);   // keep the 换装间 preview in sync with menu skin switches
    if (app.menuOwnerHwnd && IsWindow(app.menuOwnerHwnd) && app.hwnd && IsWindow(app.hwnd)) {
        if (app.pendingUpdateCheck) {
            UpdateCheckResult* pending = app.pendingUpdateCheck;
            app.pendingUpdateCheck = nullptr;
            if (!PostMessageW(app.menuOwnerHwnd, WM_UPDATE_CHECK_DONE, 0, reinterpret_cast<LPARAM>(pending))) {
                delete pending;
            }
        }
        if (app.pendingUpdateDownload) {
            UpdateDownloadResult* pending = app.pendingUpdateDownload;
            app.pendingUpdateDownload = nullptr;
            if (!PostMessageW(app.menuOwnerHwnd, WM_UPDATE_DOWNLOAD_DONE, 0, reinterpret_cast<LPARAM>(pending))) {
                delete pending;
            }
        }
    }
}

bool physicsLooksSane(const CatPhysics& p) {
    const float vals[] = {
        p.leftPaw.x.value, p.leftPaw.y.value, p.rightPaw.x.value, p.rightPaw.y.value,
        p.hindFoot.x.value, p.hindFoot.y.value, p.bodyShiftX.value, p.bodyTilt.value,
        p.bodySquash.value, p.headTilt.value, p.headYaw.value, p.headPitch.value,
        p.headOffsetY.value, p.eyeGaze.x.value, p.eyeGaze.y.value, p.pupilScale.value,
        p.huntIntensity.value, p.scruffDrag.value, p.scruffSwing.value, p.scruffSwingY.value, p.scruffTailSwing.value,
        p.douniLeftRaise.value, p.douniRightRaise.value,
        p.douniLeftKeyTarget.x.value, p.douniLeftKeyTarget.y.value,
        p.douniRightKeyTarget.x.value, p.douniRightKeyTarget.y.value,
        p.douniMouseBlend.value, p.douniLoafBlend.value,
        p.kbImpactY.value, p.kbImpactTilt.value,
        p.mouseOffset.x.value, p.mouseOffset.y.value, p.time
    };
    for (float v : vals) {
        if (!std::isfinite(v) || std::fabs(v) > 1.0e6f) return false;
    }
    return true;
}

void tickAndRender(AppState& app) {
    if (!app.isVisible || app.hiddenByFullscreen || !app.hwnd) return;

    double now = nowSeconds();
    app.lastRenderTime = now;
    float dt = (float)(now - app.lastTickTime);
    app.lastTickTime = now;
    if (dt <= 0.0f || dt > 0.2f) dt = 1.0f / 60.0f;

    // Optional menu demo typing burst
    if (app.demoActive) {
        static const PhysicalKey demoKeys[] = {
            PhysicalKey::W, PhysicalKey::A, PhysicalKey::S, PhysicalKey::D,
            PhysicalKey::F, PhysicalKey::J, PhysicalKey::O, PhysicalKey::E,
            PhysicalKey::Space, PhysicalKey::LeftShift
        };
        int targetIdx = (int)((now - app.demoStartTime) / 0.13);
        while (app.demoStepIdx <= targetIdx && app.demoStepIdx < 10) {
            app.physics.triggerKeyDown(demoKeys[app.demoStepIdx], app.settings, activeSkin(app).typingStyle == SkinTypingStyle::Douni, &app.input.activeKeys);
            app.demoStepIdx++;
        }
        if (app.demoStepIdx >= 10) app.demoActive = false;
    }

    RECT wr = {};
    GetWindowRect(app.hwnd, &wr);
    int w = std::max(1, (int)(wr.right - wr.left));
    int h = std::max(1, (int)(wr.bottom - wr.top));

    const SkinSpec& curSkin = activeSkin(app);
    bool isClassicBuiltIn = (curSkin.id == "builtin:0" || curSkin.id == "builtin:1" || curSkin.id == "builtin:2" || curSkin.id == "builtin:3");
    app.physics.step(dt, wr, app.input, app.settings,
                     curSkin.typingStyle == SkinTypingStyle::Douni,
                     curSkin.renderStyle == SkinRenderStyle::Douni,
                     isClassicBuiltIn,
                     curSkin.renderStyle == SkinRenderStyle::Beagle);
    if (!physicsLooksSane(app.physics)) {
        // A single non-finite value would otherwise make every later frame draw nothing (the cat
        // "vanishes" until restart). Rebuild the pose but keep the session statistics.
        int pets = app.physics.totalPetSessions;
        app.physics = CatPhysics();
        app.physics.totalPetSessions = pets;
    }

    // Smart Click-Through + Petting Detection
    if (!app.isDragging && app.activeResizeCorner < 0 && !app.isRotating) {
        POINT cur = {};
        GetCursorPos(&cur);
        if (PtInRect(&wr, cur)) {
            Pt rawCp = {
                (float)(cur.x - wr.left) / (float)w * CatPhysics::CanvasWidth,
                (float)(cur.y - wr.top)  / (float)h * CatPhysics::CanvasHeight
            };
            if (app.settings.isEditMode) {
                setClickThrough(app.hwnd, false);
                app.hasLastHover = false;
            } else {
                Pt cp = CatRenderer::canvasPointToCatSpace(rawCp, app.settings.flipHorizontal, app.settings.rotationDeg);
                CatHitZone zone = CatRenderer::hitTest(cp, app.physics, activeSkin(app));
                app.physics.updateTailHover(&cp);
                if (zone == CatHitZone::Head) {
                    bool skipPetForHunt = ((curSkin.renderStyle == SkinRenderStyle::Douni || curSkin.renderStyle == SkinRenderStyle::Beagle) && app.physics.huntIntensity.value > 0.20f);
                    if (!skipPetForHunt && app.hasLastHover) {
                        float d = std::hypot(cp.x - app.lastHoverCanvas.x, cp.y - app.lastHoverCanvas.y);
                        if (d > 1.2f && d < 80.0f) {
                            app.physics.registerPetStroke(cp, d, true, app.settings);
                            if (app.physics.totalPetSessions != app.lastSavedPetSessions) {
                                app.lastSavedPetSessions = app.physics.totalPetSessions;
                                app.audioThreadStop.store(true, std::memory_order_relaxed);
    app.audioThreadCv.notify_all();
    if (app.audioThread.joinable()) app.audioThread.join();

    CatSettings::saveStatsToRegistry(app.statsDay, app.input.totalKeystrokesToday, app.physics.totalPetSessions);
                            }
                        }
                    }
                    app.lastHoverCanvas = cp;
                    app.hasLastHover = true;
                } else {
                    app.hasLastHover = false;
                }
                // Holding Alt temporarily unlocks click-through so user can ALWAYS right-click or drag the cat!
                // In Green Screen mode (non-layered window), keep WS_EX_TRANSPARENT off unless fullClickThrough is active,
                // so window capture tools never skip or misread the solid green-screen window.
                bool altHeld = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                bool holdClickable = app.rButtonDown || app.pendingMenuAfterAlt;
                bool clickThrough = !holdClickable && ((app.settings.fullClickThrough && !altHeld)
                                  || (app.settings.obsCaptureMode != 2 && zone == CatHitZone::None));
                setClickThrough(app.hwnd, clickThrough);
            }
        } else {
            app.hasLastHover = false;
            app.physics.updateTailHover(nullptr);
            bool altHeld = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            bool holdClickable = app.rButtonDown || app.pendingMenuAfterAlt;
            bool clickThrough = !holdClickable && ((app.settings.fullClickThrough && !altHeld) || (app.settings.obsCaptureMode != 2));
            setClickThrough(app.hwnd, clickThrough);
        }
    }

    if (app.pendingMenuAfterAlt) {
        if ((GetAsyncKeyState(VK_MENU) & 0x8000) == 0) {
            if (app.pendingMenuAltUpTime <= 0.0) {
                app.pendingMenuAltUpTime = now;
            }
            if (now - app.pendingMenuAltUpTime >= 0.05) {
                app.pendingMenuAfterAlt = false;
                app.pendingMenuAltUpTime = 0.0;
                POINT pt = app.pendingMenuPt;
                showContextMenu(app, pt);
            }
        } else {
            app.pendingMenuAltUpTime = 0.0;
        }
    }

    RenderedPoseSnapshot curPose;
    curPose.w = w;
    curPose.h = h;
    curPose.skinId = curSkin.id;
    curPose.kbSkinId = activeKeyboardSkin(app).id;
    curPose.coatTheme = (int)app.settings.coatTheme;
    curPose.eyeTheme = (int)app.settings.eyeTheme;
    curPose.tabletMode = app.settings.tabletMode;
    curPose.obsCaptureMode = app.settings.obsCaptureMode;
    curPose.flipHorizontal = app.settings.flipHorizontal;
    curPose.rotationDeg10 = (int)std::lround(app.settings.rotationDeg * 10.0f);
    curPose.isEditMode = app.settings.isEditMode;
    curPose.showKeyOverlay = app.settings.showKeyBadges;
    curPose.showStats = app.settings.showMouseBadges;
    curPose.language = app.settings.language;
    curPose.isMusicPlaying = app.physics.isMusicPlaying;
    curPose.foregroundBlocksKeys = app.foregroundBlocksKeys;
    curPose.leftMouseDown = app.input.leftMouseDown;
    curPose.rightMouseDown = app.input.rightMouseDown;
    curPose.activeKeyCount = app.input.activeKeys.size();
    uint64_t kh = 14695981039346656037ULL;
    for (PhysicalKey k : app.input.activeKeys) {
        kh ^= (uint64_t)k;
        kh *= 1099511628211ULL;
    }
    curPose.activeKeyHash = kh;
    curPose.totalKeystrokesToday = app.input.totalKeystrokesToday;
    curPose.totalPetSessions = app.physics.totalPetSessions;
    curPose.headY10 = (int)std::lround(app.physics.headOffsetY.value * 10.0f);
    curPose.leftPawY10 = (int)std::lround(app.physics.leftPaw.y.value * 10.0f);
    curPose.rightPawY10 = (int)std::lround(app.physics.rightPaw.y.value * 10.0f);
    curPose.breath100 = (int)std::lround(app.physics.breathingOffset() * 100.0f);
    curPose.blink100 = (int)std::lround(app.physics.blinkProgress() * 100.0f);
    curPose.sleep100 = (int)std::lround(app.physics.sleepiness * 100.0f);
    curPose.tail10 = (int)std::lround(app.physics.tailTip.x.value * 10.0f + app.physics.tailPhase * 10.0f);
    curPose.purr100 = (int)std::lround(app.physics.petHeadIntensity * 100.0f);
    curPose.happy100 = (int)std::lround(app.physics.petBodyIntensity * 100.0f);
    curPose.sparkleCount = (int)app.physics.sparkles.size();

    if (!app.forceRedraw && curPose == app.lastRenderedPose) {
        return;
    }
    app.lastRenderedPose = curPose;
    app.forceRedraw = false;

    // Render 32-bit Premultiplied ARGB frame directly into DIB and present via UpdateLayeredWindow
    app.ensureBuffer(w, h);
    if (!app.memDC || !app.dibBits) return;

    Gdiplus::Bitmap bmp(w, h, w * 4, PixelFormat32bppPARGB, static_cast<BYTE*>(app.dibBits));
    Gdiplus::Graphics g(&bmp);
    g.Clear(app.settings.obsCaptureMode == 2 ? Gdiplus::Color(255, 0, 255, 0) : Gdiplus::Color(0, 0, 0, 0));
    g.ScaleTransform((float)w / CatPhysics::CanvasWidth, (float)h / CatPhysics::CanvasHeight);
    CatRenderer::draw(g, app.physics, app.settings, activeSkin(app), activeKeyboardSkin(app));

    // Green-screen mode (#00FF00) disables WS_EX_LAYERED so BitBlt / PrintWindow / Bilibili Live / OBS
    // all capture the solid window directly without black-screen issues; transparent modes use UpdateLayeredWindow.
    if (app.settings.obsCaptureMode == 2) {
        HDC wndDC = GetDC(app.hwnd);
        if (wndDC) {
            BitBlt(wndDC, 0, 0, w, h, app.memDC, 0, 0, SRCCOPY);
            ReleaseDC(app.hwnd, wndDC);
        }
        app.ulwFailures = 0;
    } else {
        POINT ptDst = { wr.left, wr.top };
        SIZE szWnd = { w, h };
        POINT ptSrc = { 0, 0 };
        BLENDFUNCTION blend = {};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;

        HDC screenDC = GetDC(nullptr);
        BOOL presented = UpdateLayeredWindow(app.hwnd, screenDC, &ptDst, &szWnd, app.memDC, &ptSrc, 0, &blend, ULW_ALPHA);
        ReleaseDC(nullptr, screenDC);
        if (presented) {
            app.ulwFailures = 0;
        } else if (++app.ulwFailures > 30) {
            // Surface lost (typically after Win+L / display reconnect): rebuild the DIB next frame.
            app.ulwFailures = 0;
            app.destroyBuffer();
        }
    }
}

double desiredFrameInterval(const AppState& app, double now) {
    if (!app.isVisible || app.hiddenByFullscreen) return 0.25; // 4 Hz when hidden
    bool active = !app.input.activeKeys.empty() ||
                  app.input.leftMouseDown || app.input.rightMouseDown || app.input.otherMouseDown ||
                  app.demoActive || app.isDragging || app.activeResizeCorner >= 0 ||
                  app.isRotating || app.pendingMenuAfterAlt ||
                  (now - app.input.lastKeyActivityTime < 1.5) ||
                  (now - app.input.lastMouseMoveTime < 1.5) ||
                  (now - app.input.lastMouseClickTime < 1.5) ||
                  app.physics.petHeadIntensity > 0.05f ||
                  app.physics.petBodyIntensity > 0.05f;
    if (active) return FRAME_INTERVAL; // 60 Hz when active
    if (app.physics.sleepiness > 0.5f) return 0.10; // 10 Hz when sleeping
    return 0.05; // 20 Hz when idle awake
}

/// Renders at most once per desiredFrameInterval; called from both the main loop and WM_TIMER (the latter
/// keeps the cat alive while a menu / message box runs its own modal loop).
void renderIfDue(AppState& app) {
    double now = nowSeconds();
    if (now < app.nextFrameTime) return;
    double interval = desiredFrameInterval(app, now);
    app.nextFrameTime = (now - app.nextFrameTime < interval) ? app.nextFrameTime + interval
                                                             : now + interval;
    tickAndRender(app);
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* app = g_app;
    switch (msg) {
        case InputEngine::WM_HOOK_KEY_EVENT:
            if (app) {
                app->input.drainHookEvents();
                app->nextFrameTime = std::min(app->nextFrameTime, nowSeconds());
            }
            return 0;
        case WM_INPUT:
            if (app) {
                app->input.handleRawInput(lParam);
                app->nextFrameTime = std::min(app->nextFrameTime, nowSeconds());
            }
            return 0;
        case WM_INPUT_DEVICE_CHANGE:
            if (app) {
                app->input.invalidateDeviceCache();
                app->forceRedraw = true;
            }
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_RENDER && app) renderIfDue(*app);
            return 0;

        case WM_WTSSESSION_CHANGE:
            if (app && hwnd == app->hwnd) {
                switch (wParam) {
                    case WTS_SESSION_LOCK:
                    case WTS_CONSOLE_DISCONNECT:
                    case WTS_REMOTE_DISCONNECT:
                        // Win+L: the key-ups for Win and L go to the secure desktop, never to us.
                        app->input.resetAllState();
                        break;
                    case WTS_SESSION_UNLOCK:
                    case WTS_SESSION_LOGON:
                    case WTS_CONSOLE_CONNECT:
                    case WTS_REMOTE_CONNECT:
                        recoverAfterSystemEvent(*app);
                        break;
                    default:
                        break;
                }
            }
            return 0;

        case WM_POWERBROADCAST:
            if (app && hwnd == app->hwnd &&
                (wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMRESUMESUSPEND)) {
                recoverAfterSystemEvent(*app);
            }
            return TRUE;

        case WM_DISPLAYCHANGE:
            // Fullscreen games often switch resolution; the cat could end up off-screen or with a
            // stale surface. Re-place it (without touching the saved position) and rebuild the buffer.
            if (app && hwnd == app->hwnd) recoverAfterSystemEvent(*app);
            return 0;

        case WM_REQUEST_QUIT:
            if (app) quitApp(*app);
            return 0;

        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                if (app && hwnd == app->hwnd && app->settings.isEditMode) {
                    RECT wr = {};
                    GetWindowRect(hwnd, &wr);
                    int w = std::max(1, (int)(wr.right - wr.left));
                    int h = std::max(1, (int)(wr.bottom - wr.top));
                    POINT cur = {};
                    GetCursorPos(&cur);
                    Pt rawCp = {
                        (float)(cur.x - wr.left) / (float)w * CatPhysics::CanvasWidth,
                        (float)(cur.y - wr.top)  / (float)h * CatPhysics::CanvasHeight
                    };
                    int hit = CatRenderer::hitTestEditOverlay(rawCp);
                    if (hit == 0 || hit == 3) { SetCursor(LoadCursorW(nullptr, IDC_SIZENWSE)); return TRUE; }
                    if (hit == 1 || hit == 2) { SetCursor(LoadCursorW(nullptr, IDC_SIZENESW)); return TRUE; }
                    if (hit == 20) { SetCursor(LoadCursorW(nullptr, IDC_HAND)); return TRUE; }
                    if (hit == 12) { SetCursor(LoadCursorW(nullptr, IDC_SIZEALL)); return TRUE; }
                    SetCursor(LoadCursorW(nullptr, IDC_ARROW));
                    return TRUE;
                }
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
            break;

        case WM_LBUTTONDBLCLK:
            if (app && hwnd == app->hwnd) {
                if (app->settings.isEditMode) {
                    RECT wr = {};
                    GetWindowRect(hwnd, &wr);
                    int w = std::max(1, (int)(wr.right - wr.left));
                    int h = std::max(1, (int)(wr.bottom - wr.top));
                    POINT cur = {};
                    GetCursorPos(&cur);
                    Pt rawCp = {
                        (float)(cur.x - wr.left) / (float)w * CatPhysics::CanvasWidth,
                        (float)(cur.y - wr.top)  / (float)h * CatPhysics::CanvasHeight
                    };
                    if (CatRenderer::hitTestEditOverlay(rawCp) == 20) {
                        app->settings.rotationDeg = 0.0f;
                        app->settings.saveToRegistry();
                        refreshSettingsWindow(*app);
                        return 0;
                    }
                }
                app->settings.isEditMode = !app->settings.isEditMode;
            }
            return 0;

        case WM_LBUTTONDOWN: {
            bool altHeld = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            if (app && hwnd == app->hwnd && (app->settings.isEditMode || !app->settings.fullClickThrough || altHeld)) {
                RECT wr = {};
                GetWindowRect(hwnd, &wr);
                int w = std::max(1, (int)(wr.right - wr.left));
                int h = std::max(1, (int)(wr.bottom - wr.top));
                POINT cur = {};
                GetCursorPos(&cur);
                if (app->settings.isEditMode) {
                    Pt rawCp = {
                        (float)(cur.x - wr.left) / (float)w * CatPhysics::CanvasWidth,
                        (float)(cur.y - wr.top)  / (float)h * CatPhysics::CanvasHeight
                    };
                    int hit = CatRenderer::hitTestEditOverlay(rawCp);
                    if (hit == 20) {
                        app->isRotating = true;
                        app->rotateMoved = false;
                        float cx = (float)wr.left + (float)w * 0.5f;
                        float cy = (float)wr.top + (float)h * 0.5f;
                        app->rotateStartAngle = std::atan2((float)cur.y - cy, (float)cur.x - cx);
                        app->rotateStartDeg = app->settings.rotationDeg;
                        SetCapture(hwnd);
                        return 0;
                    }
                    if (hit >= 0 && hit <= 3) {
                        app->activeResizeCorner = hit;
                        // Windows screen Y is top-down: anchor opposite corner
                        if (hit == 0) app->resizeAnchorScreenPt = { wr.right, wr.bottom };
                        else if (hit == 1) app->resizeAnchorScreenPt = { wr.left, wr.bottom };
                        else if (hit == 2) app->resizeAnchorScreenPt = { wr.right, wr.top };
                        else app->resizeAnchorScreenPt = { wr.left, wr.top };
                        SetCapture(hwnd);
                        return 0;
                    }
                    if (hit < 0) {
                        app->settings.isEditMode = false;
                        return 0;
                    }
                }
                app->isDragging = true;
                app->dragStartCursor = cur;
                app->dragStartWinOrigin = { wr.left, wr.top };
                app->physics.setDragging(app->settings.enableDragAnimation, 0.0f);
                SetCapture(hwnd);
            }
            return 0;
        }

        case WM_MOUSEMOVE:
            if (app && app->isRotating) {
                RECT wr = {};
                GetWindowRect(hwnd, &wr);
                int w = std::max(1, (int)(wr.right - wr.left));
                int h = std::max(1, (int)(wr.bottom - wr.top));
                POINT cur = {};
                GetCursorPos(&cur);
                float cx = (float)wr.left + (float)w * 0.5f;
                float cy = (float)wr.top + (float)h * 0.5f;
                float angle = std::atan2((float)cur.y - cy, (float)cur.x - cx);
                float deltaDeg = (angle - app->rotateStartAngle) * (180.0f / 3.14159265f);
                while (deltaDeg > 180.0f) deltaDeg -= 360.0f;
                while (deltaDeg < -180.0f) deltaDeg += 360.0f;
                if (std::fabs(deltaDeg) > 1.5f) app->rotateMoved = true;
                if (app->rotateMoved) {
                    float deg = app->rotateStartDeg + deltaDeg;
                    while (deg > 180.0f) deg -= 360.0f;
                    while (deg <= -180.0f) deg += 360.0f;
                    // Soft-snap to cardinal angles within +/-5 deg
                    const float snaps[5] = { 0.0f, 90.0f, -90.0f, 180.0f, -180.0f };
                    for (float s : snaps) {
                        if (std::fabs(deg - s) <= 5.0f) { deg = (s == -180.0f ? 180.0f : s); break; }
                    }
                    app->settings.rotationDeg = deg;
                }
                return 0;
            }
            if (app && app->activeResizeCorner >= 0) {
                POINT cur = {};
                GetCursorPos(&cur);
                float dx = std::fabs((float)(cur.x - app->resizeAnchorScreenPt.x));
                float dy = std::fabs((float)(cur.y - app->resizeAnchorScreenPt.y));
                float newScale = std::clamp(std::max(dx / CatPhysics::CanvasWidth, dy / CatPhysics::CanvasHeight), 0.45f, 2.50f);
                int nw = (int)std::round(CatPhysics::CanvasWidth * newScale);
                int nh = (int)std::round(CatPhysics::CanvasHeight * newScale);
                int nx = (app->activeResizeCorner == 0 || app->activeResizeCorner == 2)
                    ? (app->resizeAnchorScreenPt.x - nw) : app->resizeAnchorScreenPt.x;
                int ny = (app->activeResizeCorner == 0 || app->activeResizeCorner == 1)
                    ? (app->resizeAnchorScreenPt.y - nh) : app->resizeAnchorScreenPt.y;
                app->settings.scale = newScale;
                SetWindowPos(hwnd, nullptr, nx, ny, nw, nh, SWP_NOZORDER | SWP_NOACTIVATE);
                return 0;
            }
            if (app && app->isDragging) {
                POINT cur = {};
                GetCursorPos(&cur);
                RECT wr = {};
                GetWindowRect(hwnd, &wr);
                int nx = app->dragStartWinOrigin.x + (cur.x - app->dragStartCursor.x);
                int ny = app->dragStartWinOrigin.y + (cur.y - app->dragStartCursor.y);
                float stepDx = (float)(nx - wr.left);
                float stepDy = (float)(ny - wr.top);
                float rad = -app->settings.rotationDeg * (3.14159265f / 180.0f);
                float c = std::cos(rad), s = std::sin(rad);
                float localDx = stepDx * c - stepDy * s;
                float localDy = stepDx * s + stepDy * c;
                app->physics.setDragging(app->settings.enableDragAnimation, app->settings.flipHorizontal ? -localDx : localDx, localDy);
                SetWindowPos(hwnd, nullptr, nx, ny, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;

        case WM_LBUTTONUP:
            if (app && app->isRotating) {
                app->isRotating = false;
                ReleaseCapture();
                if (!app->rotateMoved) {
                    app->settings.flipHorizontal = !app->settings.flipHorizontal;
                }
                app->settings.saveToRegistry();
                refreshSettingsWindow(*app);
                return 0;
            }
            if (app && app->activeResizeCorner >= 0) {
                app->activeResizeCorner = -1;
                ReleaseCapture();
                RECT wr = {};
                GetWindowRect(hwnd, &wr);
                app->settings.posX = wr.left;
                app->settings.posY = wr.top;
                app->settings.saveToRegistry();
                refreshSettingsWindow(*app);
                return 0;
            }
            if (app && app->isDragging) {
                app->isDragging = false;
                app->physics.setDragging(false);
                ReleaseCapture();
                RECT wr = {};
                GetWindowRect(hwnd, &wr);
                app->settings.posX = wr.left;
                app->settings.posY = wr.top;
                app->settings.saveToRegistry();
                refreshSettingsWindow(*app);
            }
            return 0;

        case WM_MOUSEWHEEL:
            if (app && hwnd == app->hwnd) {
                bool altHeld = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                if (!app->settings.fullClickThrough || altHeld) {
                    short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                    if (wheelDelta != 0) {
                        float steps = (float)wheelDelta / (float)WHEEL_DELTA;
                        float stepFactor = ((GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) != 0) ? 0.08f : 0.06f;
                        float factor = 1.0f + std::clamp(steps * stepFactor, -0.25f, 0.25f);
                        app->settings.scale = std::clamp(app->settings.scale * factor, 0.45f, 2.50f);
                        applyWindowScaleAndPosition(*app, true, true);
                        refreshSettingsWindow(*app);
                    }
                }
                return 0;
            }
            break;

        case WM_RBUTTONDOWN:
            if (app && hwnd == app->hwnd) {
                bool altHeld = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                if (!app->settings.fullClickThrough || altHeld) {
                    app->rButtonDown = true;
                    SetCapture(hwnd);
                }
            }
            return 0;

        case WM_RBUTTONUP:
            if (app && hwnd == app->hwnd && app->rButtonDown) {
                app->rButtonDown = false;
                if (GetCapture() == hwnd) ReleaseCapture();
                POINT pt = {};
                GetCursorPos(&pt);
                if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) {
                    // Defer opening the popup menu until shortly after Alt is released so Windows'
                    // WM_SYSKEYUP/SC_KEYMENU finishes cleanly before TrackPopupMenu enters its modal loop.
                    app->pendingMenuAfterAlt = true;
                    app->pendingMenuAltUpTime = 0.0;
                    app->pendingMenuPt = pt;
                } else {
                    showContextMenu(*app, pt);
                }
            }
            return 0;

        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
            break;

        case WM_CANCELMODE:
        case WM_CAPTURECHANGED:
            if (app && hwnd == app->hwnd) {
                app->rButtonDown = false;
                if (app->isDragging) {
                    app->isDragging = false;
                    app->physics.setDragging(false);
                }
            }
            return 0;

        case WM_TRAYICON:
            if (app && (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP || lParam == WM_CONTEXTMENU)) {
                POINT pt = {};
                GetCursorPos(&pt);
                showContextMenu(*app, pt);
            }
            return 0;

        case WM_HOTKEY:
            if (app && wParam == HOTKEY_CLICKTHROUGH_ID) {
                app->settings.fullClickThrough = !app->settings.fullClickThrough;
                app->settings.saveToRegistry();
                if (app->settings.fullClickThrough) {
                    showTrayBalloon(*app,
                        app->settings.tr(L"KuroCat \u5DF2\u5F00\u542F\u5B8C\u5168\u9F20\u6807\u7A7F\u900F",
                                         L"KuroCat full click-through is on",
                                         L"KuroCat: ignorar cliques ativado"),
                        app->settings.tr(L"\u518D\u6B21\u6309 Ctrl+Alt+Shift+P \u6216\u6309\u4F4F Alt \u53F3\u952E\u5C0F\u732B\u5373\u53EF\u53D6\u6D88",
                                         L"Press Ctrl+Alt+Shift+P again, or hold Alt and right-click the cat, to turn it off.",
                                         L"Pressione Ctrl+Alt+Shift+P de novo ou segure Alt e clique com o botão direito no gato para desativar."));
                } else {
                    showTrayBalloon(*app,
                        app->settings.tr(L"KuroCat \u5DF2\u53D6\u6D88\u5B8C\u5168\u9F20\u6807\u7A7F\u900F",
                                         L"KuroCat full click-through is off",
                                         L"KuroCat: ignorar cliques desativado"),
                        app->settings.tr(L"\u73B0\u5728\u53EF\u4EE5\u6B63\u5E38\u62D6\u52A8\u3001\u6478\u5934\u548C\u53F3\u952E\u6253\u5F00\u83DC\u5355\u4E86",
                                         L"You can drag, pet and right-click the cat normally again.",
                                         L"Você já pode arrastar, fazer carinho e clicar com o botão direito no gato de novo."));
                }
            }
            return 0;

        case WM_WAKE_EXISTING:
            if (app) {
                // Re-launching KuroCat.exe unlocks click-through, restores visibility & opens settings menu!
                if (app->settings.fullClickThrough) {
                    app->settings.fullClickThrough = false;
                    app->settings.saveToRegistry();
                }
                if (!app->isVisible) {
                    app->isVisible = true;
                    ShowWindow(app->hwnd, SW_SHOWNOACTIVATE);
                }
                POINT pt = {};
                GetCursorPos(&pt);
                showContextMenu(*app, pt);
            }
            return 0;

        case WM_UPDATE_CHECK_DONE: {
            auto* res = reinterpret_cast<UpdateCheckResult*>(lParam);
            if (res) {
                if (app) {
                    if (res->manual) app->isManualUpdateCheck = false;
                    if (app->inContextMenu) {
                        if (app->pendingUpdateCheck && app->pendingUpdateCheck->manual && !res->manual) {
                            delete res;
                        } else {
                            delete app->pendingUpdateCheck;
                            app->pendingUpdateCheck = res;
                        }
                        return 0;
                    }
                    handleUpdateCheckResult(*app, *res);
                }
                delete res;
            }
            return 0;
        }

        case WM_UPDATE_DOWNLOAD_DONE: {
            auto* res = reinterpret_cast<UpdateDownloadResult*>(lParam);
            if (res) {
                if (app) {
                    app->isUpdating = false;
                    if (app->inContextMenu) {
                        delete app->pendingUpdateDownload;
                        app->pendingUpdateDownload = res;
                        return 0;
                    }
                    handleUpdateDownloadResult(*app, *res);
                }
                delete res;
            }
            return 0;
        }

        case WM_ERASEBKGND:
            if (app && hwnd == app->hwnd) return 1;
            break;

        case WM_PAINT:
            if (app && hwnd == app->hwnd) {
                PAINTSTRUCT ps = {};
                HDC hdc = BeginPaint(hwnd, &ps);
                if (app->memDC && app->bufW > 0 && app->bufH > 0) {
                    BitBlt(hdc, 0, 0, app->bufW, app->bufH, app->memDC, 0, 0, SRCCOPY);
                }
                EndPaint(hwnd, &ps);
                return 0;
            }
            break;

        case WM_PRINT:
        case WM_PRINTCLIENT:
            if (app && hwnd == app->hwnd) {
                HDC hdc = (HDC)wParam;
                if (hdc && app->memDC && app->bufW > 0 && app->bufH > 0) {
                    BitBlt(hdc, 0, 0, app->bufW, app->bufH, app->memDC, 0, 0, SRCCOPY);
                }
                return 0;
            }
            break;

        case WM_DROPFILES:
            // A .kurocat dropped onto the cat is imported and worn right away.
            if (app) importDroppedFiles(*app, (HDROP)wParam);
            else DragFinish((HDROP)wParam);
            return 0;

        case WM_DESTROY:
            if (hwnd == (app ? app->hwnd : nullptr)) {
                KillTimer(hwnd, TIMER_RENDER);
                if (app) {
                    if (app->sessionNotifyRegistered) {
                        WTSUnRegisterSessionNotification(hwnd);
                        app->sessionNotifyRegistered = false;
                    }
                    if (app->foregroundHook) {
                        UnhookWinEvent(app->foregroundHook);
                        app->foregroundHook = nullptr;
                    }
                    if (app->menuOwnerHwnd) {
                        UnregisterHotKey(app->menuOwnerHwnd, HOTKEY_CLICKTHROUGH_ID);
                        RemovePropW(app->menuOwnerHwnd, VERSION_PROP);
                        DestroyWindow(app->menuOwnerHwnd);
                        app->menuOwnerHwnd = nullptr;
                    }
                    Shell_NotifyIconW(NIM_DELETE, &app->nid);
                    if (app->trayIcon) DestroyIcon(app->trayIcon);
                    if (app->dressingRoom) DestroyWindow(app->dressingRoom);
                    if (app->updateWnd && IsWindow(app->updateWnd)) {
                        app->replacingUpdateWnd = true;
                        DestroyWindow(app->updateWnd);
                        app->replacingUpdateWnd = false;
                    }
                    delete app->pendingUpdateCheck;
                    app->pendingUpdateCheck = nullptr;
                    delete app->pendingUpdateDownload;
                    app->pendingUpdateDownload = nullptr;
                    app->destroyBuffer();
                }
                PostQuitMessage(0);
            }
            return 0;
    }
    if (g_msgTaskbarCreated != 0 && msg == g_msgTaskbarCreated && app && hwnd == app->menuOwnerHwnd) {
        registerTrayIcon(*app);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void CALLBACK onForegroundChanged(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD) {
    // A game just came to the front: many make themselves topmost right after showing, so
    // re-assert our own topmost state shortly afterwards instead of staying buried ("vanished").
    if (g_app && !g_app->inContextMenu) g_app->pendingTopmostTime = nowSeconds() + 0.4;
}

/// Asks the already-running KuroCat to exit and waits for it. Builds <= 1.3.2 do not know
/// WM_REQUEST_QUIT, but they shut down cleanly when their overlay window receives WM_CLOSE.
bool shutDownRunningInstance(HWND owner, DWORD timeoutMs) {
    DWORD pid = 0;
    if (owner) GetWindowThreadProcessId(owner, &pid);
    HANDLE hProc = pid ? OpenProcess(SYNCHRONIZE, FALSE, pid) : nullptr;
    if (owner) PostMessageW(owner, WM_REQUEST_QUIT, 0, 0);
    if (HWND overlay = FindWindowW(OVERLAY_CLASS, nullptr)) PostMessageW(overlay, WM_CLOSE, 0, 0);
    bool gone = false;
    if (hProc) {
        gone = (WaitForSingleObject(hProc, timeoutMs) == WAIT_OBJECT_0);
        CloseHandle(hProc);
    } else {
        for (DWORD waited = 0; waited <= timeoutMs; waited += 100) {
            if (!FindWindowW(TRAY_OWNER_CLASS, nullptr) && !FindWindowW(OVERLAY_CLASS, nullptr)) { gone = true; break; }
            Sleep(100);
        }
    }
    return gone;
}

/// Native Win32 self-update handler (no .bat scripts or hidden cmd.exe).
/// Returns true if this process was invoked solely to apply an update and should now exit.
bool handleNativeSelfUpdateIfRequested() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return false;

    DWORD oldPid = 0;
    std::wstring targetPath;
    std::wstring cleanupPath;
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], L"--apply-update") == 0 && i + 2 < argc) {
            oldPid = (DWORD)wcstoul(argv[i + 1], nullptr, 10);
            targetPath = argv[i + 2];
            i += 2;
        } else if (_wcsicmp(argv[i], L"--cleanup-update") == 0 && i + 1 < argc) {
            cleanupPath = argv[i + 1];
            i += 1;
        }
    }
    LocalFree(argv);

    if (!cleanupPath.empty()) {
        for (int attempt = 0; attempt < 10; ++attempt) {
            if (DeleteFileW(cleanupPath.c_str()) || GetLastError() == ERROR_FILE_NOT_FOUND) break;
            Sleep(150);
        }
    }

    if (targetPath.empty()) return false;

    if (oldPid != 0 && oldPid != GetCurrentProcessId()) {
        HANDLE hOld = OpenProcess(SYNCHRONIZE, FALSE, oldPid);
        if (hOld) {
            WaitForSingleObject(hOld, 12000);
            CloseHandle(hOld);
        }
    }

    wchar_t myPath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, myPath, MAX_PATH);
    std::wstring myPathStr(myPath);

    std::wstring launchPath = targetPath;
    bool copied = false;
    for (int attempt = 0; attempt < 20; ++attempt) {
        if (CopyFileW(myPathStr.c_str(), targetPath.c_str(), FALSE)) {
            copied = true;
            break;
        }
        Sleep(250);
    }

    // Per-user fallback folder if the original directory is read-only or locked
    if (!copied) {
        wchar_t localAppData[MAX_PATH] = {};
        GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
        if (localAppData[0]) {
            std::wstring fbDir = std::wstring(localAppData) + L"\\Programs";
            CreateDirectoryW(fbDir.c_str(), nullptr);
            fbDir += L"\\KuroCat";
            CreateDirectoryW(fbDir.c_str(), nullptr);
            std::wstring fbExe = fbDir + L"\\KuroCat.exe";
            if (CopyFileW(myPathStr.c_str(), fbExe.c_str(), FALSE)) {
                launchPath = fbExe;
                copied = true;
            }
        }
    }

    if (copied) {
        launchProcessArgs(launchPath, L"--takeover --cleanup-update \"" + myPathStr + L"\"");
        return true;
    }
    // If both copies failed, simply continue running from the downloaded exe itself
    return false;
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
    // Enable best available DPI awareness (Per-Monitor V2 on Win10/11, Per-Monitor on Win8.1, System DPI on Win7)
    enableBestDpiAwareness();

    if (handleNativeSelfUpdateIfRequested()) {
        return 0;
    }

    const int myVersion = versionToNumber(APP_VERSION);
    const bool forceTakeover = commandLineHasFlag(L"--takeover");   // admin relaunch
    const bool noElevate = commandLineHasFlag(L"--no-elevate");
    const wchar_t* kMutexName = L"Local\\KuroCatDesktopPetSingleInstance";

    // Single-instance guard. Re-launching the SAME version wakes the existing cat (cancels full
    // click-through and pops the menu so users never get stuck). Launching a NEWER version (e.g. a
    // freshly downloaded exe while the old one still sits in the tray or autostarts from another
    // folder) now replaces the old process instead of silently waking it — previously the old
    // version kept running and kept reporting "new version available".
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, kMutexName);
    DWORD mutexErr = GetLastError();
    bool alreadyRunning = (hMutex && mutexErr == ERROR_ALREADY_EXISTS) || (!hMutex && mutexErr == ERROR_ACCESS_DENIED);
    if (alreadyRunning) {
        HWND owner = FindWindowW(TRAY_OWNER_CLASS, nullptr);
        if (!owner && forceTakeover) {
            // The instance we replace is already shutting down; give it a moment to disappear.
            for (int i = 0; i < 50 && FindWindowW(OVERLAY_CLASS, nullptr); ++i) Sleep(100);
        } else if (!owner) {
            if (hMutex) CloseHandle(hMutex);
            return 0;
        }
        if (owner) {
            int runningVersion = (int)(INT_PTR)GetPropW(owner, VERSION_PROP);
            bool replace = forceTakeover || runningVersion < myVersion;
            if (!replace || !shutDownRunningInstance(owner, 6000)) {
                PostMessageW(owner, WM_WAKE_EXISTING, 0, 0);
                if (hMutex) CloseHandle(hMutex);
                return 0;
            }
        }
        if (!hMutex) hMutex = CreateMutexW(nullptr, FALSE, kMutexName);
    }

    AppState app;
    g_app = &app;
    app.settings.loadFromRegistry();
    kurocat::setUiLanguage(static_cast<kurocat::UiLanguage>(std::clamp(app.settings.language, 0, 2)));
    SkinLibrary::customDir() = utf8ToWide(app.settings.customSkinsDir);
    KeyboardLayout::levelKeyboard() = app.settings.levelKeyboard;
    CatSettings::loadStatsFromRegistry(app.statsDay, app.input.totalKeystrokesToday, app.physics.totalPetSessions);
    app.lastSavedPetSessions = app.physics.totalPetSessions;
    app.skins.reload();
    app.kbLibrary.reload();
    app.isElevated = isCurrentProcessElevated();

    // "Always run as administrator": hand over to an elevated copy (one UAC prompt). If the user
    // declines, simply carry on non-elevated.
    if (app.settings.runAsAdmin && !app.isElevated && !noElevate) {
        if (hMutex) { CloseHandle(hMutex); hMutex = nullptr; }
        if (relaunchSelfElevated(L"--takeover --no-elevate")) {
            app.audioThreadStop.store(true, std::memory_order_relaxed);
    app.audioThreadCv.notify_all();
    if (app.audioThread.joinable()) app.audioThread.join();

    CatSettings::saveStatsToRegistry(app.statsDay, app.input.totalKeystrokesToday, app.physics.totalPetSessions);
    g_app = nullptr;
            return 0;
        }
        hMutex = CreateMutexW(nullptr, FALSE, kMutexName);
    }

    g_msgTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    HRESULT comHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    Gdiplus::GdiplusStartupInput gdipInput;
    ULONG_PTR gdipToken = 0;
    Gdiplus::GdiplusStartup(&gdipToken, &gdipInput, nullptr);

    // Refresh auto-start exe path if user enabled auto-start and moved KuroCat.exe
    if (isAutoStartEnabled()) {
        setAutoStartEnabled(true);
    }

    app.input.onKeyDown = [&](PhysicalKey k) {
        std::string today = CatSettings::todayDateString();
        if (today != app.statsDay) {
            app.statsDay = today;
            app.input.totalKeystrokesToday = 1;
            app.audioThreadStop.store(true, std::memory_order_relaxed);
    app.audioThreadCv.notify_all();
    if (app.audioThread.joinable()) app.audioThread.join();

    CatSettings::saveStatsToRegistry(app.statsDay, app.input.totalKeystrokesToday, app.physics.totalPetSessions);
        } else if (app.input.totalKeystrokesToday % 10 == 0) {
            app.audioThreadStop.store(true, std::memory_order_relaxed);
    app.audioThreadCv.notify_all();
    if (app.audioThread.joinable()) app.audioThread.join();

    CatSettings::saveStatsToRegistry(app.statsDay, app.input.totalKeystrokesToday, app.physics.totalPetSessions);
        }
        app.physics.triggerKeyDown(k, app.settings, activeSkin(app).typingStyle == SkinTypingStyle::Douni, &app.input.activeKeys);
    };
    app.input.onMouseClick = [&](int btn, bool down) {
        app.physics.triggerMouseClick(btn, down, app.settings, activeSkin(app).renderStyle == SkinRenderStyle::Douni || activeSkin(app).renderStyle == SkinRenderStyle::Beagle);
    };

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = OVERLAY_CLASS;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    // Dedicated hidden tray/menu owner window (NO WS_EX_NOACTIVATE / WS_EX_TRANSPARENT)
    WNDCLASSEXW trayWc = {};
    trayWc.cbSize = sizeof(trayWc);
    trayWc.lpfnWndProc = wndProc;
    trayWc.hInstance = hInst;
    trayWc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    trayWc.lpszClassName = TRAY_OWNER_CLASS;
    RegisterClassExW(&trayWc);

    app.menuOwnerHwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        trayWc.lpszClassName,
        L"KuroCatTray",
        WS_POPUP,
        0, 0, 1, 1,
        nullptr, nullptr, hInst, nullptr
    );
    if (app.menuOwnerHwnd) {
        SetLayeredWindowAttributes(app.menuOwnerHwnd, 0, 0, LWA_ALPHA);
        SetPropW(app.menuOwnerHwnd, VERSION_PROP, (HANDLE)(INT_PTR)myVersion);
        // When elevated, UIPI would drop messages from the (non-elevated) taskbar and from a
        // re-launched KuroCat. Explicitly allow the few messages we rely on.
        const UINT allowed[] = { WM_TRAYICON, WM_WAKE_EXISTING, WM_REQUEST_QUIT };
        for (UINT m : allowed) ChangeWindowMessageFilterEx(app.menuOwnerHwnd, m, MSGFLT_ALLOW, nullptr);
        if (g_msgTaskbarCreated) ChangeWindowMessageFilterEx(app.menuOwnerHwnd, g_msgTaskbarCreated, MSGFLT_ALLOW, nullptr);
    }

    int initW = (int)std::round(CatPhysics::CanvasWidth * app.settings.scale);
    int initH = (int)std::round(CatPhysics::CanvasHeight * app.settings.scale);
    int initX = 0, initY = 0;
    if (app.settings.posX != -99999 && app.settings.posY != -99999) {
        initX = app.settings.posX; initY = app.settings.posY;
    } else {
        computeDefaultOrigin(initW, initH, initX, initY);
    }

    DWORD exStyle = (app.settings.obsCaptureMode != 2 ? WS_EX_LAYERED : 0)
                  | WS_EX_NOACTIVATE
                  | (app.settings.obsCaptureMode != 2 ? WS_EX_TRANSPARENT : 0)
                  | (app.settings.obsCaptureMode == 0 ? WS_EX_TOOLWINDOW : 0)
                  | (app.settings.alwaysOnTop ? WS_EX_TOPMOST : 0);

    app.hwnd = CreateWindowExW(
        exStyle,
        wc.lpszClassName,
        L"KuroCat",
        WS_POPUP,
        initX, initY, initW, initH,
        nullptr, nullptr, hInst, nullptr
    );
    if (!app.hwnd) {
        if (app.menuOwnerHwnd) DestroyWindow(app.menuOwnerHwnd);
        Gdiplus::GdiplusShutdown(gdipToken);
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    applyWindowScaleAndPosition(app, false);
    DragAcceptFiles(app.hwnd, TRUE);   // drop a .kurocat onto the cat to wear it
    allowDropsWhenElevated(app.hwnd);
    app.input.registerRawInput(app.hwnd);
    app.input.installKeyboardHook(app.hwnd);
    if (app.settings.obsCaptureMode > 0 && app.settings.obsHideDesktop) {
        applyOBSMode(app);
    }
    registerTrayIcon(app);

    // Win+L / fast-user-switch notifications (see recoverAfterSystemEvent)
    app.sessionNotifyRegistered = WTSRegisterSessionNotification(app.hwnd, NOTIFY_FOR_THIS_SESSION) != FALSE;
    // Foreground changes (games launching) -> re-assert topmost
    app.foregroundHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
                                         onForegroundChanged, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    // Global shortcut Ctrl+Alt+Shift+P to toggle full click-through anytime!
    if (app.menuOwnerHwnd) {
        RegisterHotKey(app.menuOwnerHwnd, HOTKEY_CLICKTHROUGH_ID, MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'P');
        // PT-BR fork: no automatic update check at launch.
    }

    ShowWindow(app.hwnd, SW_SHOWNOACTIVATE);
    app.lastTickTime = nowSeconds();
    tickAndRender(app);
    app.nextFrameTime = nowSeconds() + FRAME_INTERVAL;
    SetTimer(app.hwnd, TIMER_RENDER, 16, nullptr);

    app.audioPollingEnabled.store(app.settings.autoHeadphonesOnMusic, std::memory_order_relaxed);
    app.audioThread = std::thread([&app]() {
        HRESULT hrCom = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        double sustainedSince = 0.0;
        while (!app.audioThreadStop.load(std::memory_order_relaxed)) {
            if (app.audioPollingEnabled.load(std::memory_order_relaxed)) {
                double t = nowSeconds();
                if (isSystemAudioPlaying()) {
                    if (sustainedSince == 0.0) sustainedSince = t;
                    app.audioSustainedPlaying.store((t - sustainedSince) >= 1.2, std::memory_order_relaxed);
                } else {
                    sustainedSince = 0.0;
                    app.audioSustainedPlaying.store(false, std::memory_order_relaxed);
                }
            } else {
                sustainedSince = 0.0;
                app.audioSustainedPlaying.store(false, std::memory_order_relaxed);
            }
            std::unique_lock<std::mutex> lk(app.audioThreadMu);
            app.audioThreadCv.wait_for(lk, std::chrono::milliseconds(800), [&app]() {
                return app.audioThreadStop.load(std::memory_order_relaxed);
            });
        }
        if (SUCCEEDED(hrCom)) CoUninitialize();
    });

    // Frame loop. WM_TIMER is only generated when the queue is empty, so a 1000-8000 Hz gaming mouse
    // (one WM_INPUT per report) could starve it and freeze the cat. Here frames are driven by time:
    // drain messages for at most ~10 ms, then render if a frame is due. WM_TIMER still calls
    // renderIfDue() so the cat keeps animating inside menus / message boxes (their own modal loops).
    MSG msg = {};
    int exitCode = 0;
    bool running = true;
    while (running) {
        double now = nowSeconds();
        double waitSec = std::clamp(app.nextFrameTime - now, 0.0, 0.25);
        if (waitSec > 0.0) {
            MsgWaitForMultipleObjectsEx(0, nullptr, (DWORD)(waitSec * 1000.0), QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        }
        double drainUntil = nowSeconds() + 0.010;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                exitCode = (int)msg.wParam;
                break;
            }
            if (app.updateWnd && IsWindow(app.updateWnd) && msg.hwnd &&
                (msg.hwnd == app.updateWnd || IsChild(app.updateWnd, msg.hwnd)) &&
                IsDialogMessageW(app.updateWnd, &msg)) {
                if (nowSeconds() > drainUntil) break;
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (nowSeconds() > drainUntil) break;
        }
        if (!running) break;
        renderIfDue(app);
        housekeeping(app);
    }

    app.audioThreadStop.store(true, std::memory_order_relaxed);
    app.audioThreadCv.notify_all();
    if (app.audioThread.joinable()) app.audioThread.join();

    CatSettings::saveStatsToRegistry(app.statsDay, app.input.totalKeystrokesToday, app.physics.totalPetSessions);
    g_app = nullptr;
    CatRenderer::cleanupGdiplusCaches();
    Gdiplus::GdiplusShutdown(gdipToken);
    if (SUCCEEDED(comHr)) CoUninitialize();
    if (hMutex) CloseHandle(hMutex);
    return exitCode;
}
