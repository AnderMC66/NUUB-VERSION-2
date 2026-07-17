#include "infrastructure/system/WindowsClipboardService.hpp"

#include <Windows.h>

namespace nuub::infrastructure::system {

std::string WindowsClipboardService::get_clipboard() {
    if (!OpenClipboard(nullptr)) return "";

    std::string result;

    if (IsClipboardFormatAvailable(CF_TEXT)) {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData) {
            char* text = static_cast<char*>(GlobalLock(hData));
            if (text) {
                result = text;
                GlobalUnlock(hData);
            }
        }
    } else if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* text = static_cast<wchar_t*>(GlobalLock(hData));
            if (text) {
                // Convert wide string to narrow
                int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
                if (size > 0) {
                    std::string narrow(size - 1, 0);
                    WideCharToMultiByte(CP_UTF8, 0, text, -1, narrow.data(), size, nullptr, nullptr);
                    result = narrow;
                }
                GlobalUnlock(hData);
            }
        }
    }

    CloseClipboard();
    return result;
}

void WindowsClipboardService::set_clipboard(const std::string& text) {
    if (!OpenClipboard(nullptr)) return;

    EmptyClipboard();

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hMem) {
        auto* dst = static_cast<char*>(GlobalLock(hMem));
        memcpy(dst, text.c_str(), text.size() + 1);
        GlobalUnlock(hMem);
        SetClipboardData(CF_TEXT, hMem);
    }

    CloseClipboard();
}

} // namespace nuub::infrastructure::system
