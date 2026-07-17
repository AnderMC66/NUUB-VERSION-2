#pragma once

#include <string>

#include "application/interfaces/IClipboardService.hpp"

namespace nuub::infrastructure::system {

class WindowsClipboardService final : public application::interfaces::IClipboardService {
public:
    [[nodiscard]] std::string get_clipboard() override;
    void set_clipboard(const std::string& text) override;
};

} // namespace nuub::infrastructure::system
