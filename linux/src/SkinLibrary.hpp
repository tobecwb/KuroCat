#pragma once
// Imported `.kurocat` skins live in $XDG_DATA_HOME/kurocat/skins (default ~/.local/share/kurocat/skins),
// one file per skin named <content-hash>.kurocat. Built-in coats are not stored here.
// Imported `.kurokb` / `keyboard.json` keyboard skins live in $XDG_CONFIG_HOME/kurocat/keyboards
// (default ~/.config/kurocat/keyboards), named <content-hash>.kurokb.
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "../../shared/SkinSpec.hpp"
#include "../../shared/KeyboardSkinSpec.hpp"
#include "../../shared/UiLanguage.hpp"

struct SkinLibrary {
    std::vector<SkinSpec> skins; // sorted by name

    static std::string& customDir() {
        static std::string d;
        return d;
    }

    static std::string trimmedDir(std::string d) {
        while (!d.empty() && (d.back() == ' ' || d.back() == '\t' || d.back() == '\r' || d.back() == '\n')) d.pop_back();
        size_t start = 0;
        while (start < d.size() && (d[start] == ' ' || d[start] == '\t')) ++start;
        if (start > 0) d.erase(0, start);
        if (d.size() >= 2 && d[0] == '~' && d[1] == '/') {
            const char* home = std::getenv("HOME");
            if (home && home[0]) d = std::string(home) + d.substr(1);
        }
        while (d.size() > 1 && d.back() == '/') d.pop_back();
        return d;
    }

    static std::string defaultDir() {
        const char* xdg = std::getenv("XDG_DATA_HOME");
        std::string base;
        if (xdg && xdg[0] == '/') base = xdg;
        else {
            const char* home = std::getenv("HOME");
            base = std::string(home ? home : "/tmp") + "/.local/share";
        }
        return base + "/kurocat/skins";
    }

    static std::string dir() {
        std::string custom = trimmedDir(customDir());
        return custom.empty() ? defaultDir() : custom;
    }

    static bool isUsingCustomDir() {
        std::string custom = trimmedDir(customDir());
        return !custom.empty() && custom != defaultDir();
    }

    static bool ensureDir() {
        std::string d = dir();
        for (size_t i = 1; i <= d.size(); ++i) {
            if (i == d.size() || d[i] == '/') {
                std::string part = d.substr(0, i);
                if (mkdir(part.c_str(), 0755) != 0 && errno != EEXIST) return false;
            }
        }
        return true;
    }

    void reload() {
        skins.clear();
        DIR* d = opendir(dir().c_str());
        if (!d) return;
        while (dirent* e = readdir(d)) {
            std::string n = e->d_name;
            if (n.size() < 9 || n.compare(n.size() - 8, 8, ".kurocat") != 0) continue;
            SkinSpec s;
            std::string err;
            if (SkinSpec::loadFile(dir() + "/" + n, s, err)) skins.push_back(std::move(s));
            else std::fprintf(stderr, "KuroCat: skipping skin %s: %s\n", n.c_str(), err.c_str());
        }
        closedir(d);
        std::sort(skins.begin(), skins.end(), [](const SkinSpec& a, const SkinSpec& b) { return a.name < b.name; });
    }

    const SkinSpec* find(const std::string& id) const {
        for (const auto& s : skins) if (s.id == id) return &s;
        return nullptr;
    }

    // Validates `path`, copies it into the skins folder and returns the new skin's id.
    bool importFile(const std::string& path, std::string& idOut, std::string& error) {
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) { error = kurocat::uiText("无法打开这个文件", "Can't open this file", "Não foi possível abrir este arquivo"); return false; }
        std::string bytes;
        char buf[65536];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) {
            bytes.append(buf, n);
            if (bytes.size() > SkinSpec::MaxFileBytes) { std::fclose(f); error = kurocat::uiText("皮肤文件超过 2 MB 上限", "Skin file exceeds the 2 MB limit", "O arquivo da skin passa do limite de 2 MB"); return false; }
        }
        std::fclose(f);
        SkinSpec spec;
        if (!SkinSpec::parse(bytes, spec, error)) return false;
        if (!ensureDir()) { error = std::string(kurocat::uiText("无法创建皮肤文件夹 ", "Can't create the skins folder ", "Não foi possível criar a pasta de skins ")) + dir(); return false; }
        std::string target = dir() + "/" + spec.id + ".kurocat";
        std::string tmp = target + ".tmp";
        FILE* o = std::fopen(tmp.c_str(), "wb");
        if (!o) { error = std::string(kurocat::uiText("无法写入皮肤文件夹 ", "Can't write to the skins folder ", "Não foi possível gravar na pasta de skins ")) + dir(); return false; }
        bool ok = std::fwrite(bytes.data(), 1, bytes.size(), o) == bytes.size();
        ok = (std::fclose(o) == 0) && ok;
        if (!ok || std::rename(tmp.c_str(), target.c_str()) != 0) { unlink(tmp.c_str()); error = kurocat::uiText("保存皮肤失败（磁盘已满或没有权限？）", "Couldn't save the skin (disk full or no permission?)", "Não foi possível salvar a skin (disco cheio ou sem permissão?)"); return false; }
        idOut = spec.id;
        reload();
        return true;
    }

    bool remove(const std::string& id) {
        if (id.empty() || id.find('/') != std::string::npos || id.find("..") != std::string::npos) return false;
        bool ok = unlink((dir() + "/" + id + ".kurocat").c_str()) == 0;
        reload();
        return ok;
    }
};

struct KeyboardSkinLibrary {
    std::vector<KeyboardSkinSpec> skins; // sorted by name

    static std::string directory() {
        const char* xdg = std::getenv("XDG_CONFIG_HOME");
        std::string base;
        if (xdg && xdg[0] == '/') base = xdg;
        else {
            const char* home = std::getenv("HOME");
            base = std::string(home ? home : "/tmp") + "/.config";
        }
        return base + "/kurocat/keyboards";
    }

    static std::string dir() { return directory(); }

    static bool ensureDir() {
        std::string d = directory();
        for (size_t i = 1; i <= d.size(); ++i) {
            if (i == d.size() || d[i] == '/') {
                std::string part = d.substr(0, i);
                if (mkdir(part.c_str(), 0755) != 0 && errno != EEXIST) return false;
            }
        }
        return true;
    }

    static std::string lowerExt(const std::string& path) {
        size_t slash = path.find_last_of('/');
        size_t dot = path.find_last_of('.');
        if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return "";
        std::string ext = path.substr(dot + 1);
        for (char& c : ext) c = (char)std::tolower((unsigned char)c);
        return ext;
    }

    static bool looksLikeKeyboardFile(const std::string& path) {
        std::string ext = lowerExt(path);
        if (ext == "kurokb") return true;
        if (ext != "json") return false;
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) return false;
        std::string bytes;
        char buf[32768];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) {
            bytes.append(buf, n);
            if (bytes.size() > KeyboardSkinSpec::MaxFileBytes) { std::fclose(f); return false; }
        }
        std::fclose(f);
        if (bytes.empty()) return false;
        skinjson::Value root;
        std::string err;
        skinjson::Parser parser(bytes);
        if (parser.parseDocument(root, err)) {
            if (root.type != skinjson::Value::JObject) return false;
            const skinjson::Value* cols = root.get("colors");
            return cols && cols->type == skinjson::Value::JObject;
        }
        return bytes.find("\"colors\"") != std::string::npos;
    }

    void reload() {
        skins.clear();
        DIR* d = opendir(directory().c_str());
        if (!d) return;
        while (dirent* e = readdir(d)) {
            std::string n = e->d_name;
            std::string ext = lowerExt(n);
            if (ext != "kurokb" && ext != "json") continue;
            KeyboardSkinSpec s;
            std::string err;
            if (KeyboardSkinSpec::loadFile(directory() + "/" + n, s, err)) {
                bool dup = false;
                for (const auto& existing : skins) {
                    if (existing.id == s.id) { dup = true; break; }
                }
                if (!dup) skins.push_back(std::move(s));
            } else {
                std::fprintf(stderr, "KuroCat: skipping keyboard skin %s: %s\n", n.c_str(), err.c_str());
            }
        }
        closedir(d);
        std::sort(skins.begin(), skins.end(), [](const KeyboardSkinSpec& a, const KeyboardSkinSpec& b) { return a.name < b.name; });
    }

    const KeyboardSkinSpec* find(const std::string& id) const {
        if (id.empty()) return nullptr;
        for (const auto& s : skins) if (s.id == id) return &s;
        return nullptr;
    }

    // Validates `path`, copies it into the keyboards folder and returns the new skin's id.
    bool importFile(const std::string& path, std::string& outId, std::string& error) {
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) { error = kurocat::uiText("无法打开这个文件", "Can't open this file", "Não foi possível abrir este arquivo"); return false; }
        std::string bytes;
        char buf[32768];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) {
            bytes.append(buf, n);
            if (bytes.size() > KeyboardSkinSpec::MaxFileBytes) { std::fclose(f); error = kurocat::uiText("键盘皮肤文件超过 256 KB 上限", "Keyboard skin file exceeds the 256 KB limit", "O arquivo da skin de teclado passa do limite de 256 KB"); return false; }
        }
        std::fclose(f);
        KeyboardSkinSpec spec;
        if (!KeyboardSkinSpec::parse(bytes, spec, error)) return false;
        if (!ensureDir()) { error = std::string(kurocat::uiText("无法创建键盘皮肤文件夹 ", "Can't create the keyboard skins folder ", "Não foi possível criar a pasta de skins de teclado ")) + directory(); return false; }
        std::string target = directory() + "/" + spec.id + ".kurokb";
        std::string tmp = target + ".tmp";
        FILE* o = std::fopen(tmp.c_str(), "wb");
        if (!o) { error = std::string(kurocat::uiText("无法写入键盘皮肤文件夹 ", "Can't write to the keyboard skins folder ", "Não foi possível gravar na pasta de skins de teclado ")) + directory(); return false; }
        bool ok = std::fwrite(bytes.data(), 1, bytes.size(), o) == bytes.size();
        ok = (std::fclose(o) == 0) && ok;
        if (!ok || std::rename(tmp.c_str(), target.c_str()) != 0) {
            unlink(tmp.c_str());
            error = kurocat::uiText("保存键盘皮肤失败（磁盘已满或没有权限？）", "Couldn't save the keyboard skin (disk full or no permission?)", "Não foi possível salvar a skin de teclado (disco cheio ou sem permissão?)");
            return false;
        }
        outId = spec.id;
        reload();
        return true;
    }

    bool remove(const std::string& id) {
        if (id.size() != 16 || id.find_first_not_of("0123456789abcdef") != std::string::npos) return false;
        bool ok = unlink((directory() + "/" + id + ".kurokb").c_str()) == 0;
        reload();
        return ok;
    }
};
