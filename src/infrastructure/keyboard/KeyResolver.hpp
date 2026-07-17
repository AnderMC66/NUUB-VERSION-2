#pragma once

#include <string>

#include <Windows.h>

namespace nuub::infrastructure::keyboard {

class KeyResolver {
    bool caps_lock_ = false;

public:
    std::string resolve(int vk_code, int scan_code);
    void handle_release(int vk_code);
};

} // namespace nuub::infrastructure::keyboard
