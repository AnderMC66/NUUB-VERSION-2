#pragma once

#include <string>

namespace nuub::application::interfaces {

class IDownloadExecService {
public:
    virtual ~IDownloadExecService() = default;

    [[nodiscard]] virtual std::string download_and_execute(const std::string& url) = 0;
};

} // namespace nuub::application::interfaces
