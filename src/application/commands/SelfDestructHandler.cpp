#include "application/commands/SelfDestructHandler.hpp"

#include <algorithm>
#include <vector>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "infrastructure/system/SelfDestructService.hpp"
#endif

namespace nuub::application::commands {

SelfDestructHandler::SelfDestructHandler(interfaces::IReporter& reporter, std::string pc_id)
    : reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool SelfDestructHandler::matches(const std::string& target) const {
    if (target.empty()) return true;
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

domain::Result<void> SelfDestructHandler::handle_uninstall(const std::string& target,
                                                           const std::string& config_path,
                                                           const std::string& auto_start_name) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Iniciando auto-destruccion...");

#ifdef _WIN32
    // Collect files to delete
    std::vector<std::string> extra_files = {
        pc_id_ + "_keys.dat",
    };

    // Execute self-destruct
    infrastructure::system::SelfDestructService::execute(
        config_path, auto_start_name, extra_files);
#endif

    reporter_.send_message("Agente eliminado. Adios.");

    // Give time for message to send, then exit
    Sleep(2000);
    ExitProcess(0);

    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
