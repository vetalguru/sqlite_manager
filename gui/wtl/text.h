#ifndef SQLITE_MANAGER_GUI_WTL_TEXT_H
#define SQLITE_MANAGER_GUI_WTL_TEXT_H

#include <windows.h>

#include <string>

namespace sqlite_manager_gui::wtl {

// SQLite (and gui/core) carry text as UTF-8 std::string; the Win32 controls
// are UTF-16. These convert between the two at the boundary.

inline std::wstring Widen(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    const int size = ::MultiByteToWideChar(
        CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<size_t>(size), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                          static_cast<int>(utf8.size()), wide.data(), size);
    return wide;
}

inline std::string Narrow(const std::wstring& wide) {
    if (wide.empty()) return std::string();
    const int size = ::WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                                           static_cast<int>(wide.size()),
                                           nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<size_t>(size), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                          static_cast<int>(wide.size()), utf8.data(), size,
                          nullptr, nullptr);
    return utf8;
}

}  // namespace sqlite_manager_gui::wtl

#endif  // SQLITE_MANAGER_GUI_WTL_TEXT_H
