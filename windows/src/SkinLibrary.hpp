#pragma once
// Imported `.kurocat` skins live in %APPDATA%\KuroCat\skins, one file per skin named <content-hash>.kurocat.
// Built-in coats are not stored here.
#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <string>
#include <vector>
#include "../../shared/SkinSpec.hpp"
#include "../../shared/KeyboardSkinSpec.hpp"
#include "../../shared/UiLanguage.hpp"

inline std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w((size_t)std::max(0, n), L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
    return w;
}

inline std::string wideToUtf8(const std::wstring& w) {
    if (w.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s((size_t)std::max(0, n), '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

struct SkinLibrary {
    std::vector<SkinSpec> skins; // sorted by name

    static std::wstring baseDir() {
        wchar_t path[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path)) && path[0]) return std::wstring(path) + L"\\KuroCat";
        wchar_t env[MAX_PATH] = {0};
        DWORD n = GetEnvironmentVariableW(L"APPDATA", env, MAX_PATH);
        return (n > 0 && n < MAX_PATH ? std::wstring(env) : std::wstring(L".")) + L"\\KuroCat";
    }
    static std::wstring& customDir() {
        static std::wstring d;
        return d;
    }
    static std::wstring trimmedDir(std::wstring d) {
        while (!d.empty() && (d.back() == L' ' || d.back() == L'\t' || d.back() == L'\r' || d.back() == L'\n')) d.pop_back();
        size_t start = 0;
        while (start < d.size() && (d[start] == L' ' || d[start] == L'\t')) ++start;
        if (start > 0) d.erase(0, start);
        while (d.size() > 3 && (d.back() == L'\\' || d.back() == L'/')) d.pop_back();
        return d;
    }
    static std::wstring defaultDir() { return baseDir() + L"\\skins"; }
    static std::wstring dir() {
        std::wstring custom = trimmedDir(customDir());
        return custom.empty() ? defaultDir() : custom;
    }
    static bool isUsingCustomDir() {
        std::wstring custom = trimmedDir(customDir());
        return !custom.empty() && _wcsicmp(custom.c_str(), defaultDir().c_str()) != 0;
    }

    static bool ensureDir() {
        std::wstring d = dir();
        int rc = SHCreateDirectoryExW(nullptr, d.c_str(), nullptr);
        if (rc == ERROR_SUCCESS || rc == ERROR_ALREADY_EXISTS || rc == ERROR_FILE_EXISTS) return true;
        DWORD attrs = GetFileAttributesW(d.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    static bool readFile(const std::wstring& path, std::string& bytes, std::string& error) {
        HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) { error = kurocat::uiText("无法打开这个文件", "Couldn't open this file", "Não foi possível abrir este arquivo"); return false; }
        LARGE_INTEGER size = {};
        if (!GetFileSizeEx(h, &size) || size.QuadPart > (LONGLONG)SkinSpec::MaxFileBytes) {
            CloseHandle(h);
            error = size.QuadPart > (LONGLONG)SkinSpec::MaxFileBytes
                ? kurocat::uiText("皮肤文件超过 2 MB 上限", "Skin file exceeds the 2 MB limit", "O arquivo de skin passa do limite de 2 MB")
                : kurocat::uiText("无法读取这个文件", "Couldn't read this file", "Não foi possível ler este arquivo");
            return false;
        }
        bytes.assign((size_t)size.QuadPart, '\0');
        DWORD read = 0;
        BOOL ok = bytes.empty() || ReadFile(h, &bytes[0], (DWORD)bytes.size(), &read, nullptr);
        CloseHandle(h);
        if (!ok || read != bytes.size()) { error = kurocat::uiText("无法读取这个文件", "Couldn't read this file", "Não foi possível ler este arquivo"); return false; }
        return true;
    }

    void reload() {
        skins.clear();
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW((dir() + L"\\*.kurocat").c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) return;
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            std::string bytes, err;
            SkinSpec s;
            if (readFile(dir() + L"\\" + fd.cFileName, bytes, err) && SkinSpec::parse(bytes, s, err)) skins.push_back(std::move(s));
        } while (FindNextFileW(h, &fd));
        FindClose(h);
        std::sort(skins.begin(), skins.end(), [](const SkinSpec& a, const SkinSpec& b) { return a.name < b.name; });
    }

    const SkinSpec* find(const std::string& id) const {
        for (const auto& s : skins) if (s.id == id) return &s;
        return nullptr;
    }

    // Validates `path`, copies it into the skins folder and returns the new skin's id.
    bool importFile(const std::wstring& path, std::string& idOut, std::string& error) {
        std::string bytes;
        if (!readFile(path, bytes, error)) return false;
        SkinSpec spec;
        if (!SkinSpec::parse(bytes, spec, error)) return false;
        if (!ensureDir()) { error = std::string(kurocat::uiText("无法创建皮肤文件夹 ", "Couldn't create the skins folder ", "Não foi possível criar a pasta de skins ")) + wideToUtf8(dir()); return false; }
        std::wstring target = dir() + L"\\" + utf8ToWide(spec.id) + L".kurocat";
        std::wstring tmp = target + L".tmp";
        HANDLE h = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) { error = std::string(kurocat::uiText("无法写入皮肤文件夹 ", "Couldn't write to the skins folder ", "Não foi possível gravar na pasta de skins ")) + wideToUtf8(dir()); return false; }
        DWORD written = 0;
        BOOL ok = WriteFile(h, bytes.data(), (DWORD)bytes.size(), &written, nullptr) && written == bytes.size();
        ok = CloseHandle(h) && ok;
        if (!ok || !MoveFileExW(tmp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            DeleteFileW(tmp.c_str());
            error = kurocat::uiText("保存皮肤失败（磁盘已满或没有权限？）", "Couldn't save the skin (disk full or no permission?)", "Falha ao salvar a skin (disco cheio ou sem permissão?)");
            return false;
        }
        idOut = spec.id;
        reload();
        return true;
    }

    bool remove(const std::string& id) {
        if (id.size() != 16 || id.find_first_not_of("0123456789abcdef") != std::string::npos) return false;
        bool ok = DeleteFileW((dir() + L"\\" + utf8ToWide(id) + L".kurocat").c_str()) != FALSE;
        reload();
        return ok;
    }

    // Disambiguates duplicate skin names with a short hash suffix so users never confuse them.
    std::string displayName(const SkinSpec& s) const {
        int dup = 0;
        for (const auto& item : skins) if (item.name == s.name) ++dup;
        if (dup > 1 && s.id.size() >= 4) return s.name + " (#" + s.id.substr(0, 4) + ")";
        return s.name;
    }

    static std::string jsonEscapeString(const std::string& s) {
        std::string out = "\"";
        for (unsigned char c : s) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20) {
                        char buf[7];
                        std::snprintf(buf, sizeof buf, "\\u%04x", (unsigned)c);
                        out += buf;
                    } else {
                        out += (char)c;
                    }
                    break;
            }
        }
        out += '"';
        return out;
    }

    // Replaces the top-level JSON "name" string value in-place while preserving all other fields byte-for-byte.
    static bool replaceTopLevelName(const std::string& src, const std::string& newName, std::string& out) {
        size_t i = 0, n = src.size();
        if (n >= 3 && (uint8_t)src[0] == 0xEF && (uint8_t)src[1] == 0xBB && (uint8_t)src[2] == 0xBF) i = 3;
        auto skipWs = [&]() { while (i < n && (src[i] == ' ' || src[i] == '\t' || src[i] == '\r' || src[i] == '\n')) ++i; };
        auto skipString = [&](std::string* rawOut = nullptr) -> bool {
            if (i >= n || src[i] != '"') return false;
            size_t start = i++;
            while (i < n) {
                char c = src[i++];
                if (c == '\\') { if (i < n) ++i; continue; }
                if (c == '"') { if (rawOut) *rawOut = src.substr(start + 1, i - start - 2); return true; }
            }
            return false;
        };
        skipWs();
        if (i >= n || src[i] != '{') return false;
        ++i;
        int depth = 1;
        size_t valStart = std::string::npos, valEnd = std::string::npos;
        while (i < n && depth > 0) {
            skipWs();
            if (i >= n) break;
            if (depth == 1 && src[i] == '"') {
                std::string key;
                if (!skipString(&key)) return false;
                skipWs();
                if (i < n && src[i] == ':') {
                    ++i; skipWs();
                    if (key == "name" && i < n && src[i] == '"') {
                        size_t s = i;
                        if (!skipString(nullptr)) return false;
                        valStart = s;
                        valEnd = i;
                    }
                }
                continue;
            }
            char c = src[i];
            if (c == '"') { if (!skipString(nullptr)) return false; continue; }
            if (c == '{' || c == '[') ++depth;
            else if (c == '}' || c == ']') --depth;
            ++i;
        }
        if (valStart == std::string::npos) return false;
        out = src.substr(0, valStart) + jsonEscapeString(newName) + src.substr(valEnd);
        return true;
    }

    bool rename(const std::string& id, const std::string& rawNewName, std::string& newIdOut, std::string& error) {
        if (id.size() != 16 || id.find_first_not_of("0123456789abcdef") != std::string::npos) {
            error = kurocat::uiText("无效的皮肤 ID", "Invalid skin ID", "ID de skin inválido");
            return false;
        }
        std::string cleaned = skinparse::trimmedUtf8(rawNewName, 40);
        if (cleaned.empty()) {
            error = kurocat::uiText("皮肤名称不能为空", "Skin name cannot be empty", "O nome da skin não pode ficar vazio");
            return false;
        }
        std::wstring oldPath = dir() + L"\\" + utf8ToWide(id) + L".kurocat";
        std::string bytes;
        if (!readFile(oldPath, bytes, error)) return false;
        std::string updated;
        if (!replaceTopLevelName(bytes, cleaned, updated)) {
            error = kurocat::uiText("无法定位皮肤 JSON 中的 name 字段", "Couldn't find the name field in the skin JSON", "Não foi possível encontrar o campo name no JSON da skin");
            return false;
        }
        SkinSpec checkSpec;
        if (!SkinSpec::parse(updated, checkSpec, error)) return false;
        if (!ensureDir()) { error = kurocat::uiText("无法访问皮肤文件夹", "Couldn't access the skins folder", "Não foi possível acessar a pasta de skins"); return false; }
        std::wstring newPath = dir() + L"\\" + utf8ToWide(checkSpec.id) + L".kurocat";
        std::wstring tmp = newPath + L".tmp";
        HANDLE h = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) { error = kurocat::uiText("无法写入皮肤文件", "Couldn't write the skin file", "Não foi possível gravar o arquivo de skin"); return false; }
        DWORD written = 0;
        BOOL ok = WriteFile(h, updated.data(), (DWORD)updated.size(), &written, nullptr) && written == updated.size();
        ok = CloseHandle(h) && ok;
        if (!ok || !MoveFileExW(tmp.c_str(), newPath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            DeleteFileW(tmp.c_str());
            error = kurocat::uiText("保存重命名后的皮肤失败", "Couldn't save the renamed skin", "Falha ao salvar a skin renomeada");
            return false;
        }
        if (checkSpec.id != id) DeleteFileW(oldPath.c_str());
        newIdOut = checkSpec.id;
        reload();
        return true;
    }
};

// Imported `.kurokb` / keyboard `.json` skins live in %APPDATA%\KuroCat\keyboards, one file per skin named <content-hash>.kurokb.
struct KeyboardSkinLibrary {
    std::vector<KeyboardSkinSpec> skins; // sorted by name

    static std::wstring directory() { return SkinLibrary::baseDir() + L"\\keyboards"; }

    static bool ensureDir() {
        for (const std::wstring& d : {SkinLibrary::baseDir(), directory()}) {
            if (!CreateDirectoryW(d.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
        }
        return true;
    }

    static bool readFile(const std::wstring& path, std::string& bytes, std::string& error) {
        HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) { error = kurocat::uiText("无法打开这个文件", "Couldn't open this file", "Não foi possível abrir este arquivo"); return false; }
        LARGE_INTEGER size = {};
        if (!GetFileSizeEx(h, &size) || size.QuadPart > (LONGLONG)KeyboardSkinSpec::MaxFileBytes) {
            CloseHandle(h);
            error = size.QuadPart > (LONGLONG)KeyboardSkinSpec::MaxFileBytes
                ? kurocat::uiText("键盘皮肤文件超过 256 KB 上限", "Keyboard skin file exceeds the 256 KB limit", "O arquivo de skin de teclado passa do limite de 256 KB")
                : kurocat::uiText("无法读取这个文件", "Couldn't read this file", "Não foi possível ler este arquivo");
            return false;
        }
        bytes.assign((size_t)size.QuadPart, '\0');
        DWORD read = 0;
        BOOL ok = bytes.empty() || ReadFile(h, &bytes[0], (DWORD)bytes.size(), &read, nullptr);
        CloseHandle(h);
        if (!ok || read != bytes.size()) { error = kurocat::uiText("无法读取这个文件", "Couldn't read this file", "Não foi possível ler este arquivo"); return false; }
        return true;
    }

    // Quick sniffer to distinguish a keyboard skin (.kurokb or .json with "colors" object) from a cat .kurocat skin.
    static bool looksLikeKeyboardFile(const std::wstring& path) {
        size_t slash = path.find_last_of(L"\\/");
        size_t dot = path.find_last_of(L'.');
        if (dot == std::wstring::npos || (slash != std::wstring::npos && dot < slash)) return false;
        std::wstring ext = path.substr(dot + 1);
        for (wchar_t& ch : ext) {
            if (ch >= L'A' && ch <= L'Z') ch = (wchar_t)(ch - L'A' + L'a');
        }
        if (ext == L"kurokb") return true;
        if (ext != L"json") return false;
        std::string bytes, err;
        if (!readFile(path, bytes, err)) return false;
        skinjson::Value root;
        skinjson::Parser parser(bytes);
        if (!parser.parseDocument(root, err) || root.type != skinjson::Value::JObject) return false;
        const skinjson::Value* cols = root.get("colors");
        return cols != nullptr && cols->type == skinjson::Value::JObject;
    }

    void reload() {
        skins.clear();
        auto loadPattern = [&](const wchar_t* pattern) {
            WIN32_FIND_DATAW fd;
            HANDLE h = FindFirstFileW((directory() + pattern).c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE) return;
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                std::string bytes, err;
                KeyboardSkinSpec s;
                if (readFile(directory() + L"\\" + fd.cFileName, bytes, err) && KeyboardSkinSpec::parse(bytes, s, err)) {
                    bool dup = false;
                    for (const auto& existing : skins) {
                        if (existing.id == s.id) { dup = true; break; }
                    }
                    if (!dup) skins.push_back(std::move(s));
                }
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        };
        loadPattern(L"\\*.kurokb");
        loadPattern(L"\\*.json");
        std::sort(skins.begin(), skins.end(), [](const KeyboardSkinSpec& a, const KeyboardSkinSpec& b) { return a.name < b.name; });
    }

    const KeyboardSkinSpec* find(const std::string& id) const {
        if (id.empty()) return nullptr;
        for (const auto& s : skins) if (s.id == id) return &s;
        return nullptr;
    }

    // Validates `path`, copies it into the keyboards folder and returns the new keyboard skin's id.
    bool importFile(const std::wstring& path, std::string& idOut, std::string& error) {
        std::string bytes;
        if (!readFile(path, bytes, error)) return false;
        KeyboardSkinSpec spec;
        if (!KeyboardSkinSpec::parse(bytes, spec, error)) return false;
        if (!ensureDir()) { error = kurocat::uiText("无法创建键盘皮肤文件夹 %APPDATA%\\KuroCat\\keyboards", "Couldn't create the keyboard skins folder %APPDATA%\\KuroCat\\keyboards", "Não foi possível criar a pasta de skins de teclado %APPDATA%\\KuroCat\\keyboards"); return false; }
        std::wstring target = directory() + L"\\" + utf8ToWide(spec.id) + L".kurokb";
        std::wstring tmp = target + L".tmp";
        HANDLE h = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) { error = kurocat::uiText("无法写入键盘皮肤文件夹 %APPDATA%\\KuroCat\\keyboards", "Couldn't write to the keyboard skins folder %APPDATA%\\KuroCat\\keyboards", "Não foi possível gravar na pasta de skins de teclado %APPDATA%\\KuroCat\\keyboards"); return false; }
        DWORD written = 0;
        BOOL ok = WriteFile(h, bytes.data(), (DWORD)bytes.size(), &written, nullptr) && written == bytes.size();
        ok = CloseHandle(h) && ok;
        if (!ok || !MoveFileExW(tmp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            DeleteFileW(tmp.c_str());
            error = kurocat::uiText("保存键盘皮肤失败（磁盘已满或没有权限？）", "Couldn't save the keyboard skin (disk full or no permission?)", "Falha ao salvar a skin de teclado (disco cheio ou sem permissão?)");
            return false;
        }
        idOut = spec.id;
        reload();
        return true;
    }

    bool remove(const std::string& id) {
        if (id.size() != 16 || id.find_first_not_of("0123456789abcdef") != std::string::npos) return false;
        bool ok = DeleteFileW((directory() + L"\\" + utf8ToWide(id) + L".kurokb").c_str()) != FALSE;
        reload();
        return ok;
    }
};
