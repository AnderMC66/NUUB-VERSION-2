#include "application/commands/ProcessHandler.hpp"

#include <algorithm>
#include <stdexcept>

namespace nuub::application::commands {

ProcessHandler::ProcessHandler(
    interfaces::IProcessService& process,
    interfaces::IReporter& reporter,
    std::string pc_id)
    : process_(process)
    , reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool ProcessHandler::matches(const std::string& target) const {
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

domain::Result<void> ProcessHandler::handle_ps(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    auto procs = process_.list_processes();
    reporter_.send_message(procs);
    return domain::Result<void>::success();
}

domain::Result<void> ProcessHandler::handle_kill(const std::string& target, const std::string& pid_str) {
    if (!matches(target)) return domain::Result<void>::success();

    if (pid_str.empty()) {
        reporter_.send_message("Uso: /kill [target] <pid>");
        return domain::Result<void>::success();
    }

    int pid = 0;
    try {
        size_t pos = 0;
        pid = std::stoi(pid_str, &pos);
        if (pos != pid_str.size() || pid <= 0) {
            reporter_.send_message("PID invalido: " + pid_str);
            return domain::Result<void>::success();
        }
    } catch (const std::exception&) {
        reporter_.send_message("PID invalido: " + pid_str);
        return domain::Result<void>::success();
    }

    if (process_.kill_process(pid)) {
        reporter_.send_message("Proceso " + std::to_string(pid) + " terminado.");
    } else {
        reporter_.send_message("No se pudo terminar el proceso " + std::to_string(pid) + ".");
    }
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
