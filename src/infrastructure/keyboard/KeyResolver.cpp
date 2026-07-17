#include "infrastructure/keyboard/KeyResolver.hpp"

namespace nuub::infrastructure::keyboard {

std::string KeyResolver::resolve(int vk_code, int scan_code) {
    if (vk_code == VK_CAPITAL) {
        caps_lock_ = !caps_lock_;
        return {};
    }

    // Ignore modifier keys
    if (vk_code == VK_SHIFT || vk_code == VK_LSHIFT || vk_code == VK_RSHIFT ||
        vk_code == VK_CONTROL || vk_code == VK_LCONTROL || vk_code == VK_RCONTROL ||
        vk_code == VK_MENU || vk_code == VK_LMENU || vk_code == VK_RMENU ||
        vk_code == VK_LWIN || vk_code == VK_RWIN) {
        return {};
    }

    // Special keys
    switch (vk_code) {
        case VK_SPACE:   return " ";
        case VK_RETURN:  return "\n";
        case VK_TAB:     return "\t";
        case VK_BACK:    return "\x08";
        case VK_ESCAPE:  return " [escape] ";
        case VK_DELETE:  return " [delete] ";
        case VK_UP:      return " [up] ";
        case VK_DOWN:    return " [down] ";
        case VK_LEFT:    return " [left] ";
        case VK_RIGHT:   return " [right] ";
        case VK_HOME:    return " [home] ";
        case VK_END:     return " [end] ";
        case VK_PRIOR:   return " [pageup] ";
        case VK_NEXT:    return " [pagedown] ";
        case VK_F1:  case VK_F2:  case VK_F3:  case VK_F4:
        case VK_F5:  case VK_F6:  case VK_F7:  case VK_F8:
        case VK_F9:  case VK_F10: case VK_F11: case VK_F12:
            return " [f" + std::to_string(vk_code - VK_F1 + 1) + "] ";
    }

    // Get keyboard state for shift/caps detection
    BYTE key_state[256]{};
    GetKeyboardState(key_state);
    bool shift = (key_state[VK_SHIFT] & 0x80) != 0;
    bool caps = caps_lock_;

    // Alphanumeric characters
    if (vk_code >= '0' && vk_code <= '9') {
        if (shift) {
            const char* shifted = ")!@#$%^&*(";
            return std::string(1, shifted[vk_code - '0']);
        }
        return std::string(1, static_cast<char>(vk_code));
    }

    if (vk_code >= 'A' && vk_code <= 'Z') {
        bool upper = shift ^ caps;
        char c = static_cast<char>(vk_code);
        return std::string(1, upper ? c : static_cast<char>(c + 32));
    }

    // Other keys: represent as [keyname]
    char buf[64]{};
    int result = GetKeyNameTextA(static_cast<LONG>(scan_code) << 16, buf, sizeof(buf));
    if (result > 0) {
        std::string name(buf);
        for (auto& ch : name) ch = static_cast<char>(::tolower(ch));
        return " [" + name + "] ";
    }

    return {};
}

void KeyResolver::handle_release(int vk_code) {
    // Track held keys for modifier state if needed in the future
    (void)vk_code;
}

} // namespace nuub::infrastructure::keyboard
