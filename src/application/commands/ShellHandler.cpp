#include "application/commands/ShellHandler.hpp"

#include <algorithm>

namespace nuub::application::commands {

ShellHandler::ShellHandler(
    interfaces::IShellService& shell,
    interfaces::IReporter& reporter,
    std::string pc_id)
    : shell_(shell)
    , reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool ShellHandler::matches(const std::string& target) const {
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

domain::Result<void> ShellHandler::handle_shell(const std::string& target, const std::string& command) {
    if (!matches(target)) return domain::Result<void>::success();

    if (command.empty()) {
        reporter_.send_message("Uso: /shell [target] <comando>");
        return domain::Result<void>::success();
    }

    reporter_.send_message("Ejecutando: " + command);
    auto output = shell_.execute(command);
    reporter_.send_message(output);
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
