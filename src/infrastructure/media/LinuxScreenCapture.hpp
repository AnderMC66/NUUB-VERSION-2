#pragma once

#include <optional>
#include <string>

namespace nuub::infrastructure::media {

class LinuxScreenCapture {
    std::string pc_id_;

public:
    explicit LinuxScreenCapture(std::string pc_id);
    [[nodiscard]] std::optional<std::string> take_screenshot();
};

} // namespace nuub::infrastructure::media
