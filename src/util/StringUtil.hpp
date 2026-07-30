#pragma once
// ============================================================
// StringUtil.hpp — wstring / UTF-8 string 変換ユーティリティ
// ============================================================
// 使用場所:
//   - Win32 API (wchar_t) ↔ WebView2 ExecuteScript (UTF-8) の橋渡し
//   - RapidJSON (char) ↔ Win32 API (wchar_t) の橋渡し
// ============================================================

#include <string>
#include <string_view>
#include <windows.h>

namespace util {

// ------------------------------------------------
// wstring (UTF-16) → string (UTF-8)
// ------------------------------------------------
inline std::string WideToUtf8(std::wstring_view wide) {
    if (wide.empty()) return {};
    const int size = ::WideCharToMultiByte(
        CP_UTF8, 0,
        wide.data(), static_cast<int>(wide.size()),
        nullptr, 0, nullptr, nullptr
    );
    std::string result(size, '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0,
        wide.data(), static_cast<int>(wide.size()),
        result.data(), size, nullptr, nullptr
    );
    return result;
}

// ------------------------------------------------
// string (UTF-8) → wstring (UTF-16)
// ------------------------------------------------
inline std::wstring Utf8ToWide(std::string_view utf8) {
    if (utf8.empty()) return {};
    const int size = ::MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.data(), static_cast<int>(utf8.size()),
        nullptr, 0
    );
    std::wstring result(size, L'\0');
    ::MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.data(), static_cast<int>(utf8.size()),
        result.data(), size
    );
    return result;
}

// ------------------------------------------------
// string → wstring (ANSI / CP_ACP — 内部使用のみ)
// ------------------------------------------------
inline std::wstring AnsiToWide(std::string_view ansi) {
    if (ansi.empty()) return {};
    const int size = ::MultiByteToWideChar(
        CP_ACP, 0,
        ansi.data(), static_cast<int>(ansi.size()),
        nullptr, 0
    );
    std::wstring result(size, L'\0');
    ::MultiByteToWideChar(
        CP_ACP, 0,
        ansi.data(), static_cast<int>(ansi.size()),
        result.data(), size
    );
    return result;
}

// ------------------------------------------------
// JavaScriptの文字列エスケープ (シングルクォート内)
// ------------------------------------------------
inline std::string EscapeForJs(std::string_view s) {
    std::string result;
    result.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '\'': result += "\\'";  break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            default:   result += c;      break;
        }
    }
    return result;
}

} // namespace util
