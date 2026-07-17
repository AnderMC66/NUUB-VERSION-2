#pragma once

#include <string>

#include "application/interfaces/IDownloadExecService.hpp"

namespace nuub::infrastructure::system {

class WindowsDownloadExecService final : public application::interfaces::IDownloadExecService {
public:
    [[nodiscard]] std::string download_and_execute(const std::string& url) override;
};

} // namespace nuub::infrastructure::system
