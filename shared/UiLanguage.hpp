#pragma once
// Process-wide UI language for the Windows and Linux builds.
// The platform settings code calls setUiLanguage() after loading settings and
// whenever the user picks another language; shared code (skin validation
// messages) reads it through uiText().

namespace kurocat {

enum class UiLanguage : int { Chinese = 0, English = 1, Portuguese = 2 };

inline UiLanguage& uiLanguageStorage() {
    static UiLanguage lang = UiLanguage::Portuguese;
    return lang;
}

inline void setUiLanguage(UiLanguage lang) { uiLanguageStorage() = lang; }
inline UiLanguage uiLanguage() { return uiLanguageStorage(); }

/// Picks the UTF-8 string for the current UI language.
inline const char* uiText(const char* zh, const char* en, const char* pt) {
    switch (uiLanguage()) {
        case UiLanguage::English: return en;
        case UiLanguage::Portuguese: return pt;
        default: return zh;
    }
}

} // namespace kurocat
