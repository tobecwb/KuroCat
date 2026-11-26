#pragma once
#define NOMINMAX
#include <windows.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <deque>
#include <functional>
#include <algorithm>
#include <chrono>
#include <string>
#include <cwctype>
#include <thread>
#include <mutex>
#include <atomic>
#include "KeyMap.hpp"

/// High-precision monotonic clock in seconds.
inline double nowSeconds() {
    static const auto start = std::chrono::steady_clock::now();
    auto cur = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(cur - start).count();
}

/// Hardware-level full-state keyboard & mouse engine via Windows Raw Input API (RIDEV_INPUTSINK).
/// Captures true N-key rollover make/break scancodes across typing (any IME) and games!
class InputEngine {
public:
    std::unordered_set<PhysicalKey> activeKeys;
    std::vector<PhysicalKey> pressedOrder;

    bool leftMouseDown = false;
    bool rightMouseDown = false;
    bool otherMouseDown = false;
    double lastMouseMoveTime = 0.0;
    double lastMouseClickTime = 0.0;
    double lastKeyActivityTime = 0.0;

    POINT lastCursorPos = { 0, 0 };
    Pt mouseVelocity = { 0.0f, 0.0f };
    float rawDeltaAccumX = 0.0f;
    float rawDeltaAccumY = 0.0f;

    int totalKeystrokesToday = 0;
    bool isRawInputRegistered = false;
    HHOOK kbHook_ = nullptr; // kept for compatibility; managed on hookThread_
    static constexpr UINT WM_HOOK_KEY_EVENT = WM_APP + 61;
    static constexpr UINT WM_HOOK_REINSTALL = WM_APP + 62;

    std::function<void(PhysicalKey)> onKeyDown;
    std::function<void(PhysicalKey)> onKeyUp;
    std::function<void(int, bool)> onMouseClick;

    static inline InputEngine* s_hookInstance = nullptr;

    static LRESULT CALLBACK lowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && s_hookInstance && lParam) {
            const KBDLLHOOKSTRUCT* kb = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
            bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
            bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
            if (isDown || isUp) {
                USHORT flags = (kb->flags & LLKHF_EXTENDED) ? RI_KEY_E0 : 0;
                PhysicalKey key = mapRawInputKey((USHORT)kb->scanCode, flags, (USHORT)kb->vkCode);
                if (key != PhysicalKey::None) {
                    s_hookInstance->enqueueHookKeyEvent(key, isDown);
                }
            }
        }
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    ~InputEngine() {
        uninstallKeyboardHook();
    }

    void installKeyboardHook(HWND notifyHwnd = nullptr) {
        if (notifyHwnd) hookNotifyHwnd_.store(notifyHwnd, std::memory_order_relaxed);
        s_hookInstance = this;
        if (hookThread_.joinable()) {
            DWORD tid = hookThreadId_.load(std::memory_order_acquire);
            if (tid != 0 && !kbHookActive_.load(std::memory_order_acquire)) {
                PostThreadMessageW(tid, WM_HOOK_REINSTALL, 0, 0);
            }
            return;
        }
        hookThreadId_.store(0, std::memory_order_relaxed);
        hookThread_ = std::thread([this]() {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
            MSG msg = {};
            PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE);
            hookThreadId_.store(GetCurrentThreadId(), std::memory_order_release);

            HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, lowLevelKeyboardProc, GetModuleHandleW(nullptr), 0);
            kbHook_ = hook;
            kbHookActive_.store(hook != nullptr, std::memory_order_release);

            while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
                if (msg.message == WM_HOOK_REINSTALL && !hook) {
                    hook = SetWindowsHookExW(WH_KEYBOARD_LL, lowLevelKeyboardProc, GetModuleHandleW(nullptr), 0);
                    kbHook_ = hook;
                    kbHookActive_.store(hook != nullptr, std::memory_order_release);
                }
            }
            if (hook) {
                UnhookWindowsHookEx(hook);
                hook = nullptr;
            }
            kbHook_ = nullptr;
            kbHookActive_.store(false, std::memory_order_release);
            hookThreadId_.store(0, std::memory_order_release);
        });
    }

    void uninstallKeyboardHook() {
        if (hookThread_.joinable()) {
            DWORD tid = hookThreadId_.load(std::memory_order_acquire);
            if (tid != 0) {
                PostThreadMessageW(tid, WM_QUIT, 0, 0);
            } else {
                for (int i = 0; i < 50; ++i) {
                    tid = hookThreadId_.load(std::memory_order_acquire);
                    if (tid != 0) {
                        PostThreadMessageW(tid, WM_QUIT, 0, 0);
                        break;
                    }
                    Sleep(1);
                }
            }
            hookThread_.join();
        }
        if (s_hookInstance == this) s_hookInstance = nullptr;
    }

    void drainHookEvents() {
        std::vector<HookKeyEvent> local;
        {
            std::lock_guard<std::mutex> lk(hookQueueMu_);
            if (hookQueue_.empty()) return;
            local.swap(hookQueue_);
        }
        for (const HookKeyEvent& ev : local) {
            setRawKeyState(ev.key, ev.isDown);
        }
    }

    bool registerRawInput(HWND hwnd) {
        RAWINPUTDEVICE rid[3] = {};
        // Keyboard (Generic Desktop 0x01, Usage 0x06)
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage = 0x06;
        rid[0].dwFlags = RIDEV_INPUTSINK;
        rid[0].hwndTarget = hwnd;
        // Mouse (Generic Desktop 0x01, Usage 0x02)
        rid[1].usUsagePage = 0x01;
        rid[1].usUsage = 0x02;
        rid[1].dwFlags = RIDEV_INPUTSINK;
        rid[1].hwndTarget = hwnd;
        // Graphics Tablet / Pen Digitizer (Digitizer 0x0D, Usage 0x02 Pen)
        rid[2].usUsagePage = 0x0D;
        rid[2].usUsage = 0x02;
        rid[2].dwFlags = RIDEV_INPUTSINK;
        rid[2].hwndTarget = hwnd;

        isRawInputRegistered = (RegisterRawInputDevices(rid, 3, sizeof(RAWINPUTDEVICE)) != FALSE);
        if (!isRawInputRegistered) {
            isRawInputRegistered = (RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE)) != FALSE);
        }
        refreshTabletPresence(nowSeconds());
        GetCursorPos(&lastCursorPos);
        return isRawInputRegistered;
    }

    bool isTabletConnected() const {
        if (tabletHardwareFound_) return true;
        if (lastTabletEventTime_ <= 0.0) return false;
        double now = nowSeconds();
        if (now - lastTabletEventTime_ >= 20.0) return false;
        // If no external physical tablet is connected and regular mouse input resumed, return cleanly to mouse
        if (lastRegularMouseEventTime_ > lastTabletEventTime_ + 1.2) return false;
        return true;
    }

    static bool isKnownTabletVid(DWORD vid) {
        switch (vid) {
            case 0x056A: // Wacom
            case 0x256C: // Huion / Gaomon
            case 0x28BD: // XP-Pen / UGEE
            case 0x5543: // UC-Logic / UGEE
            case 0x2FEB: // Veikk
            case 0x2179: // Hanvon
            case 0x33F8: // Xencelabs
            case 0x2FBB: // Parblo
                return true;
            default: return false;
        }
    }

    static bool isExcludedNonTabletVid(DWORD vid) {
        switch (vid) {
            case 0x05AC: // Apple USB
            case 0x004C: // Apple Bluetooth (Magic Trackpad)
            case 0x1B96: // N-Trig built-in laptop touchscreen
            case 0x045E: // Microsoft Surface built-in digitizer
            case 0x06CB: // Synaptics Precision Touchpad
            case 0x04F3: // ELAN laptop touchpad/touchscreen
            case 0x0488: // Cirque touchpad
            case 0x27C6: // Goodix touchscreen
            case 0x222A: // ILI Technology touchscreen
                return true;
            default: return false;
        }
    }

    static bool isExternalPhysicalDevicePath(HANDLE hDevice) {
        UINT nameLen = 0;
        if (GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, nullptr, &nameLen) != 0 || nameLen == 0) {
            return false;
        }
        std::wstring path(nameLen, L'\0');
        if (GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, path.data(), &nameLen) == (UINT)-1) {
            return false;
        }
        for (wchar_t& ch : path) ch = (wchar_t)towupper(ch);
        // Reject virtual / root-enumerated / ACPI / I2C built-in laptop digitizers & remote-desktop virtual HIDs
        if (path.find(L"ROOT#") != std::wstring::npos ||
            path.find(L"ACPI#") != std::wstring::npos ||
            path.find(L"SWD#") != std::wstring::npos ||
            path.find(L"VIRTUAL") != std::wstring::npos ||
            path.find(L"VHID") != std::wstring::npos ||
            path.find(L"RDP_") != std::wstring::npos ||
            path.find(L"I2C") != std::wstring::npos) {
            return false;
        }
        if (path.find(L"VEN_") != std::wstring::npos && path.find(L"USB#") == std::wstring::npos) {
            return false;
        }
        return (path.find(L"USB#") != std::wstring::npos ||
                path.find(L"VID_") != std::wstring::npos ||
                path.find(L"BTH") != std::wstring::npos);
    }

    static bool isExternalTabletDevice(HANDLE hDevice) {
        RID_DEVICE_INFO info = {};
        info.cbSize = sizeof(info);
        UINT cb = sizeof(info);
        if (GetRawInputDeviceInfoW(hDevice, RIDI_DEVICEINFO, &info, &cb) == (UINT)-1) return false;
        if (info.dwType != RIM_TYPEHID) return false;
        if (isExcludedNonTabletVid(info.hid.dwVendorId)) return false;
        if (!isExternalPhysicalDevicePath(hDevice)) return false;
        // Known dedicated graphics tablet vendor: require Digitizer (0x0D) or vendor-specific (>= 0xFF00) interface
        if (isKnownTabletVid(info.hid.dwVendorId)) {
            return info.hid.usUsagePage == 0x0D || info.hid.usUsagePage >= 0xFF00;
        }
        // Unknown external USB/BT vendor: require EXPLICIT Pen usage (0x02) — NEVER generic Digitizer (0x01) / TouchScreen (0x04) / TouchPad (0x05)
        return info.hid.usUsagePage == 0x0D && info.hid.usUsage == 0x02;
    }

    static bool isVerifiedPenEventDevice(HANDLE hDevice) {
        RID_DEVICE_INFO info = {};
        info.cbSize = sizeof(info);
        UINT cb = sizeof(info);
        if (GetRawInputDeviceInfoW(hDevice, RIDI_DEVICEINFO, &info, &cb) == (UINT)-1) return false;
        if (info.dwType != RIM_TYPEHID) return false;
        if (info.hid.usUsagePage != 0x0D || info.hid.usUsage != 0x02) return false;
        UINT nameLen = 0;
        if (GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, nullptr, &nameLen) == 0 && nameLen > 0) {
            std::wstring path(nameLen, L'\0');
            if (GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, path.data(), &nameLen) != (UINT)-1) {
                for (wchar_t& ch : path) ch = (wchar_t)towupper(ch);
                if (path.find(L"ROOT#") != std::wstring::npos ||
                    path.find(L"VIRTUAL") != std::wstring::npos ||
                    path.find(L"VHID") != std::wstring::npos ||
                    path.find(L"RDP_") != std::wstring::npos) {
                    return false;
                }
            }
        }
        return true;
    }

    void refreshTabletPresence(double now) {
        lastTabletScanTime_ = now;
        UINT numDevices = 0;
        if (GetRawInputDeviceList(nullptr, &numDevices, sizeof(RAWINPUTDEVICELIST)) != 0 || numDevices == 0) {
            tabletHardwareFound_ = false;
            return;
        }
        std::vector<RAWINPUTDEVICELIST> list(numDevices);
        if (GetRawInputDeviceList(list.data(), &numDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1) {
            tabletHardwareFound_ = false;
            return;
        }
        bool found = false;
        for (UINT i = 0; i < numDevices; ++i) {
            if (list[i].dwType != RIM_TYPEHID) continue;
            if (isExternalTabletDevice(list[i].hDevice)) {
                found = true;
                break;
            }
        }
        tabletHardwareFound_ = found;
    }

    /// Drops every held key / mouse button and re-syncs the cursor. Called after Win+L unlock,
    /// sleep/resume and display changes: the key-up events for Win+L (and anything held while the
    /// secure desktop was up) never reach us, so stale state would otherwise pin the cat's paws.
    void resetAllState() {
        std::vector<PhysicalKey> held(activeKeys.begin(), activeKeys.end());
        rawKeys_.clear();
        asyncKeys_.clear();
        rawKeyDownTime_.clear();
        for (PhysicalKey k : held) recomputeUnifiedKeys(k);
        activeKeys.clear();
        pressedOrder.clear();
        if (leftMouseDown) updateMouseButton(0, false);
        if (rightMouseDown) updateMouseButton(1, false);
        if (otherMouseDown) updateMouseButton(2, false);
        rawDeltaAccumX = 0.0f;
        rawDeltaAccumY = 0.0f;
        mouseVelocity = { 0.0f, 0.0f };
        GetCursorPos(&lastCursorPos);
        lastRawKbTime_ = 0.0;
        lastRawMouseTime_ = 0.0;
        lastRawLeftBtnTime_ = 0.0;
        lastRawRightBtnTime_ = 0.0;
        lastRawOtherBtnTime_ = 0.0;
    }

    /// Seconds since the last keyboard WM_INPUT arrived (used to spot UIPI-blocked games).
    double secondsSinceRawKeyboard(double now) const {
        return lastRawKbTime_ > 0.0 ? now - lastRawKbTime_ : 1e9;
    }

    void handleRawInput(LPARAM lParam) {
        UINT size = 0;
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0) {
            return;
        }
        // Gaming mice can post 1000–8000 WM_INPUT/s: avoid a heap allocation per message.
        alignas(8) BYTE stackBuf[256];
        std::vector<BYTE> heapBuf;
        BYTE* data = stackBuf;
        if (size > sizeof(stackBuf)) {
            heapBuf.resize(size);
            data = heapBuf.data();
        }
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, data, &size, sizeof(RAWINPUTHEADER)) != size) {
            return;
        }

        const RAWINPUT* raw = reinterpret_cast<const RAWINPUT*>(data);
        if (raw->header.dwType == RIM_TYPEKEYBOARD) {
            const RAWKEYBOARD& kb = raw->data.keyboard;
            // Ignore synthetic overrun / pause prefix
            if (kb.MakeCode == 0 && kb.VKey == 0xFF) return;
            PhysicalKey key = mapRawInputKey(kb.MakeCode, kb.Flags, kb.VKey);
            if (key == PhysicalKey::None) return;

            bool isBreak = (kb.Flags & RI_KEY_BREAK) != 0;
            lastRawKbTime_ = nowSeconds();
            setRawKeyState(key, !isBreak);
        } else if (raw->header.dwType == RIM_TYPEMOUSE) {
            const RAWMOUSE& m = raw->data.mouse;
            double now = nowSeconds();
            lastRawMouseTime_ = now;
            if ((m.usFlags & MOUSE_MOVE_ABSOLUTE) == 0 && (m.lLastX != 0 || m.lLastY != 0)) {
                rawDeltaAccumX += (float)m.lLastX;
                rawDeltaAccumY += (float)m.lLastY;
                lastMouseMoveTime = now;
                lastRegularMouseEventTime_ = now;
            }
            if (m.usButtonFlags != 0) lastRegularMouseEventTime_ = now;
            if (m.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)   { lastRawLeftBtnTime_ = now;  updateMouseButton(0, true); }
            if (m.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)     { lastRawLeftBtnTime_ = now;  updateMouseButton(0, false); }
            if (m.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)  { lastRawRightBtnTime_ = now; updateMouseButton(1, true); }
            if (m.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)    { lastRawRightBtnTime_ = now; updateMouseButton(1, false); }
            if (m.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) { lastRawOtherBtnTime_ = now; updateMouseButton(2, true); }
            if (m.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)   { lastRawOtherBtnTime_ = now; updateMouseButton(2, false); }
            if (m.usButtonFlags & RI_MOUSE_WHEEL)              lastMouseMoveTime = now;
        } else if (raw->header.dwType == RIM_TYPEHID) {
            if (raw->data.hid.dwSizeHid > 0 && raw->data.hid.dwCount > 0) {
                HANDLE hDev = raw->header.hDevice;
                auto it = verifiedPenDeviceCache_.find(hDev);
                bool isPen = false;
                if (it != verifiedPenDeviceCache_.end()) {
                    isPen = it->second;
                } else {
                    if (verifiedPenDeviceCache_.size() > 64) verifiedPenDeviceCache_.clear();
                    isPen = isVerifiedPenEventDevice(hDev);
                    verifiedPenDeviceCache_[hDev] = isPen;
                }
                if (isPen) {
                    double now = nowSeconds();
                    lastTabletEventTime_ = now;
                    lastMouseMoveTime = now;
                }
            }
        }
    }

    /// Called at 60Hz from main loop to compute smooth cursor velocity (works in desktop & locked-cursor games!).
    void pollMouse(float dt) {
        double now = nowSeconds();
        if (now - lastTabletScanTime_ > 2.5) {
            refreshTabletPresence(now);
        }
        POINT cur = lastCursorPos;
        GetCursorPos(&cur);
        float dx = (float)(cur.x - lastCursorPos.x);
        float dy = (float)(cur.y - lastCursorPos.y);
        lastCursorPos = cur;

        // If cursor is locked by a game (dx==0, dy==0), fall back to raw mouse hardware deltas!
        if (std::fabs(dx) < 0.5f && std::fabs(dy) < 0.5f && (std::fabs(rawDeltaAccumX) > 0.5f || std::fabs(rawDeltaAccumY) > 0.5f)) {
            dx = rawDeltaAccumX;
            dy = rawDeltaAccumY;
        }
        rawDeltaAccumX = 0.0f;
        rawDeltaAccumY = 0.0f;

        float dist = std::hypot(dx, dy);
        if (dist > 0.4f && dt > 0.001f) {
            lastMouseMoveTime = nowSeconds();
            mouseVelocity.x = dx / dt;
            mouseVelocity.y = dy / dt;
        } else {
            mouseVelocity.x *= 0.82f;
            mouseVelocity.y *= 0.82f;
        }

        now = nowSeconds();
        pollAsyncFallback(now);
        while (!recentKeyTimes_.empty() && now - recentKeyTimes_.front() > 10.0) {
            recentKeyTimes_.pop_front();
        }
    }

    int currentKPM() const {
        return (int)recentKeyTimes_.size() * 6;
    }

    void invalidateDeviceCache() {
        verifiedPenDeviceCache_.clear();
    }

private:
    struct HookKeyEvent {
        PhysicalKey key;
        bool isDown;
    };

    void enqueueHookKeyEvent(PhysicalKey key, bool isDown) {
        {
            std::lock_guard<std::mutex> lk(hookQueueMu_);
            if (hookQueue_.size() < 256) {
                hookQueue_.push_back({key, isDown});
            }
        }
        HWND h = hookNotifyHwnd_.load(std::memory_order_relaxed);
        if (h) PostMessageW(h, WM_HOOK_KEY_EVENT, 0, 0);
    }

    std::thread hookThread_;
    std::atomic<DWORD> hookThreadId_{0};
    std::atomic<bool> kbHookActive_{false};
    std::atomic<HWND> hookNotifyHwnd_{nullptr};
    std::mutex hookQueueMu_;
    std::vector<HookKeyEvent> hookQueue_;

    std::deque<double> recentKeyTimes_;
    std::unordered_set<PhysicalKey> rawKeys_;
    std::unordered_set<PhysicalKey> asyncKeys_;
    std::unordered_map<PhysicalKey, double> rawKeyDownTime_;
    std::unordered_map<HANDLE, bool> verifiedPenDeviceCache_;
    double lastRawKbTime_ = 0.0;
    double lastRawMouseTime_ = 0.0;
    double lastRawLeftBtnTime_ = 0.0;
    double lastRawRightBtnTime_ = 0.0;
    double lastRawOtherBtnTime_ = 0.0;
    bool tabletHardwareFound_ = false;
    double lastTabletEventTime_ = 0.0;
    double lastRegularMouseEventTime_ = 0.0;
    double lastTabletScanTime_ = 0.0;

    void setRawKeyState(PhysicalKey key, bool isDown) {
        if (isDown) {
            rawKeys_.insert(key);
            rawKeyDownTime_[key] = nowSeconds();
        } else {
            rawKeys_.erase(key);
            asyncKeys_.erase(key);
            rawKeyDownTime_.erase(key);
        }
        recomputeUnifiedKeys(key);
    }

    static int physicalKeyToVk(PhysicalKey k) {
        switch (k) {
            case PhysicalKey::A: return 'A'; case PhysicalKey::B: return 'B'; case PhysicalKey::C: return 'C';
            case PhysicalKey::D: return 'D'; case PhysicalKey::E: return 'E'; case PhysicalKey::F: return 'F';
            case PhysicalKey::G: return 'G'; case PhysicalKey::H: return 'H'; case PhysicalKey::I: return 'I';
            case PhysicalKey::J: return 'J'; case PhysicalKey::K: return 'K'; case PhysicalKey::L: return 'L';
            case PhysicalKey::M: return 'M'; case PhysicalKey::N: return 'N'; case PhysicalKey::O: return 'O';
            case PhysicalKey::P: return 'P'; case PhysicalKey::Q: return 'Q'; case PhysicalKey::R: return 'R';
            case PhysicalKey::S: return 'S'; case PhysicalKey::T: return 'T'; case PhysicalKey::U: return 'U';
            case PhysicalKey::V: return 'V'; case PhysicalKey::W: return 'W'; case PhysicalKey::X: return 'X';
            case PhysicalKey::Y: return 'Y'; case PhysicalKey::Z: return 'Z';
            case PhysicalKey::K0: return '0'; case PhysicalKey::K1: return '1'; case PhysicalKey::K2: return '2';
            case PhysicalKey::K3: return '3'; case PhysicalKey::K4: return '4'; case PhysicalKey::K5: return '5';
            case PhysicalKey::K6: return '6'; case PhysicalKey::K7: return '7'; case PhysicalKey::K8: return '8';
            case PhysicalKey::K9: return '9';
            case PhysicalKey::Space: return VK_SPACE; case PhysicalKey::Enter: return VK_RETURN;
            case PhysicalKey::Backspace: return VK_BACK; case PhysicalKey::Tab: return VK_TAB;
            case PhysicalKey::Escape: return VK_ESCAPE; case PhysicalKey::CapsLock: return VK_CAPITAL;
            case PhysicalKey::LeftShift: return VK_LSHIFT; case PhysicalKey::RightShift: return VK_RSHIFT;
            case PhysicalKey::LeftControl: return VK_LCONTROL; case PhysicalKey::RightControl: return VK_RCONTROL;
            case PhysicalKey::LeftOption: return VK_LMENU; case PhysicalKey::RightOption: return VK_RMENU;
            case PhysicalKey::LeftCommand: return VK_LWIN; case PhysicalKey::RightCommand: return VK_RWIN;
            case PhysicalKey::ArrowUp: return VK_UP; case PhysicalKey::ArrowDown: return VK_DOWN;
            case PhysicalKey::ArrowLeft: return VK_LEFT; case PhysicalKey::ArrowRight: return VK_RIGHT;
            case PhysicalKey::F1: return VK_F1; case PhysicalKey::F2: return VK_F2; case PhysicalKey::F3: return VK_F3;
            case PhysicalKey::F4: return VK_F4; case PhysicalKey::F5: return VK_F5; case PhysicalKey::F6: return VK_F6;
            case PhysicalKey::F7: return VK_F7; case PhysicalKey::F8: return VK_F8; case PhysicalKey::F9: return VK_F9;
            case PhysicalKey::F10: return VK_F10; case PhysicalKey::F11: return VK_F11; case PhysicalKey::F12: return VK_F12;
            case PhysicalKey::Minus: return VK_OEM_MINUS; case PhysicalKey::Equal: return VK_OEM_PLUS;
            case PhysicalKey::LeftBracket: return VK_OEM_4; case PhysicalKey::RightBracket: return VK_OEM_6;
            case PhysicalKey::Backslash: return VK_OEM_5; case PhysicalKey::Semicolon: return VK_OEM_1;
            case PhysicalKey::Quote: return VK_OEM_7; case PhysicalKey::Comma: return VK_OEM_COMMA;
            case PhysicalKey::Period: return VK_OEM_PERIOD; case PhysicalKey::Slash: return VK_OEM_2;
            case PhysicalKey::Grave: return VK_OEM_3; case PhysicalKey::ForwardDelete: return VK_DELETE;
            default: return 0;
        }
    }

    void pollAsyncFallback(double now) {
        struct VkEntry { int vk; PhysicalKey key; };
        static constexpr VkEntry kTable[] = {
            {'A', PhysicalKey::A}, {'B', PhysicalKey::B}, {'C', PhysicalKey::C}, {'D', PhysicalKey::D},
            {'E', PhysicalKey::E}, {'F', PhysicalKey::F}, {'G', PhysicalKey::G}, {'H', PhysicalKey::H},
            {'I', PhysicalKey::I}, {'J', PhysicalKey::J}, {'K', PhysicalKey::K}, {'L', PhysicalKey::L},
            {'M', PhysicalKey::M}, {'N', PhysicalKey::N}, {'O', PhysicalKey::O}, {'P', PhysicalKey::P},
            {'Q', PhysicalKey::Q}, {'R', PhysicalKey::R}, {'S', PhysicalKey::S}, {'T', PhysicalKey::T},
            {'U', PhysicalKey::U}, {'V', PhysicalKey::V}, {'W', PhysicalKey::W}, {'X', PhysicalKey::X},
            {'Y', PhysicalKey::Y}, {'Z', PhysicalKey::Z},
            {'0', PhysicalKey::K0}, {'1', PhysicalKey::K1}, {'2', PhysicalKey::K2}, {'3', PhysicalKey::K3},
            {'4', PhysicalKey::K4}, {'5', PhysicalKey::K5}, {'6', PhysicalKey::K6}, {'7', PhysicalKey::K7},
            {'8', PhysicalKey::K8}, {'9', PhysicalKey::K9},
            {VK_SPACE, PhysicalKey::Space}, {VK_RETURN, PhysicalKey::Enter}, {VK_BACK, PhysicalKey::Backspace},
            {VK_TAB, PhysicalKey::Tab}, {VK_ESCAPE, PhysicalKey::Escape}, {VK_CAPITAL, PhysicalKey::CapsLock},
            {VK_LSHIFT, PhysicalKey::LeftShift}, {VK_RSHIFT, PhysicalKey::RightShift},
            {VK_LCONTROL, PhysicalKey::LeftControl}, {VK_RCONTROL, PhysicalKey::RightControl},
            {VK_LMENU, PhysicalKey::LeftOption}, {VK_RMENU, PhysicalKey::RightOption},
            {VK_LWIN, PhysicalKey::LeftCommand}, {VK_RWIN, PhysicalKey::RightCommand},
            {VK_UP, PhysicalKey::ArrowUp}, {VK_DOWN, PhysicalKey::ArrowDown},
            {VK_LEFT, PhysicalKey::ArrowLeft}, {VK_RIGHT, PhysicalKey::ArrowRight},
            {VK_F1, PhysicalKey::F1}, {VK_F2, PhysicalKey::F2}, {VK_F3, PhysicalKey::F3}, {VK_F4, PhysicalKey::F4},
            {VK_F5, PhysicalKey::F5}, {VK_F6, PhysicalKey::F6}, {VK_F7, PhysicalKey::F7}, {VK_F8, PhysicalKey::F8},
            {VK_F9, PhysicalKey::F9}, {VK_F10, PhysicalKey::F10}, {VK_F11, PhysicalKey::F11}, {VK_F12, PhysicalKey::F12},
            {VK_OEM_MINUS, PhysicalKey::Minus}, {VK_OEM_PLUS, PhysicalKey::Equal},
            {VK_OEM_4, PhysicalKey::LeftBracket}, {VK_OEM_6, PhysicalKey::RightBracket},
            {VK_OEM_5, PhysicalKey::Backslash}, {VK_OEM_1, PhysicalKey::Semicolon},
            {VK_OEM_7, PhysicalKey::Quote}, {VK_OEM_COMMA, PhysicalKey::Comma},
            {VK_OEM_PERIOD, PhysicalKey::Period}, {VK_OEM_2, PhysicalKey::Slash},
            {VK_OEM_3, PhysicalKey::Grave}, {VK_DELETE, PhysicalKey::ForwardDelete}
        };

        bool rawIdle = (now - lastRawKbTime_) > 0.20;
        for (const auto& e : kTable) {
            bool isDown = (GetAsyncKeyState(e.vk) & 0x8000) != 0;
            bool inRaw = rawKeys_.count(e.key) > 0;
            bool inAsync = asyncKeys_.count(e.key) > 0;
            if (isDown && !inRaw && !inAsync) {
                asyncKeys_.insert(e.key);
                recomputeUnifiedKeys(e.key);
            }
        }

        // Verify every currently active/raw/async key (including keys not mapped in kTable or lost under UIPI/focus shifts).
        std::vector<PhysicalKey> tracked;
        tracked.reserve(activeKeys.size() + rawKeys_.size() + asyncKeys_.size());
        for (PhysicalKey k : activeKeys) tracked.push_back(k);
        for (PhysicalKey k : rawKeys_) if (!activeKeys.count(k)) tracked.push_back(k);
        for (PhysicalKey k : asyncKeys_) if (!activeKeys.count(k) && !rawKeys_.count(k)) tracked.push_back(k);

        for (PhysicalKey k : tracked) {
            int vk = physicalKeyToVk(k);
            bool isDown = (vk != 0) ? ((GetAsyncKeyState(vk) & 0x8000) != 0) : false;
            bool inRaw = rawKeys_.count(k) > 0;
            bool inAsync = asyncKeys_.count(k) > 0;
            double downAge = 1e9;
            if (auto it = rawKeyDownTime_.find(k); it != rawKeyDownTime_.end()) {
                downAge = now - it->second;
            }
            bool rawStale = inRaw && (rawIdle || downAge > 0.20);
            bool capStale = (k == PhysicalKey::CapsLock) && (downAge > 0.45);
            if (capStale || (!isDown && (inAsync || rawStale || (!inRaw && activeKeys.count(k) > 0)))) {
                asyncKeys_.erase(k);
                rawKeys_.erase(k);
                rawKeyDownTime_.erase(k);
                recomputeUnifiedKeys(k);
            }
        }

        // Reconcile each mouse button using its own button-event timestamp (not cursor motion!)
        if (leftMouseDown) {
            bool lDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            if (!lDown && (now - lastRawLeftBtnTime_) > 0.20) updateMouseButton(0, false);
        } else if ((now - lastRawMouseTime_) > 0.25 && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0) {
            updateMouseButton(0, true);
        }
        if (rightMouseDown) {
            bool rDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
            if (!rDown && (now - lastRawRightBtnTime_) > 0.20) updateMouseButton(1, false);
        } else if ((now - lastRawMouseTime_) > 0.25 && (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) {
            updateMouseButton(1, true);
        }
        if (otherMouseDown) {
            bool mDown = ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0)
                      || ((GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0)
                      || ((GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0);
            if (!mDown && (now - lastRawOtherBtnTime_) > 0.20) updateMouseButton(2, false);
        } else if ((now - lastRawMouseTime_) > 0.25 && (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0) {
            updateMouseButton(2, true);
        }
    }

    void recomputeUnifiedKeys(PhysicalKey key) {
        bool wasActive = activeKeys.count(key) > 0;
        bool isNowActive = (rawKeys_.count(key) > 0) || (asyncKeys_.count(key) > 0);
        if (isNowActive && !wasActive) {
            activeKeys.insert(key);
            pressedOrder.erase(std::remove(pressedOrder.begin(), pressedOrder.end(), key), pressedOrder.end());
            pressedOrder.push_back(key);
            double now = nowSeconds();
            lastKeyActivityTime = now;
            recentKeyTimes_.push_back(now);
            totalKeystrokesToday++;
            if (onKeyDown) onKeyDown(key);
        } else if (!isNowActive && wasActive) {
            activeKeys.erase(key);
            pressedOrder.erase(std::remove(pressedOrder.begin(), pressedOrder.end(), key), pressedOrder.end());
            if (onKeyUp) onKeyUp(key);
        }
    }

    void updateMouseButton(int button, bool isDown) {
        bool prev = false;
        if (button == 0) { prev = leftMouseDown; leftMouseDown = isDown; }
        else if (button == 1) { prev = rightMouseDown; rightMouseDown = isDown; }
        else { prev = otherMouseDown; otherMouseDown = isDown; }

        if (prev != isDown) {
            double now = nowSeconds();
            lastMouseMoveTime = now;
            if (isDown) lastMouseClickTime = now;
            if (onMouseClick) onMouseClick(button, isDown);
        }
    }
};
