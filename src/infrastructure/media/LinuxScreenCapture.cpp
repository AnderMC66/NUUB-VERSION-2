#include "infrastructure/media/LinuxScreenCapture.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>

namespace nuub::infrastructure::media {

LinuxScreenCapture::LinuxScreenCapture(std::string pc_id)
    : pc_id_(std::move(pc_id)) {}

std::optional<std::string> LinuxScreenCapture::take_screenshot() {
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::string path = "screenshot_" + pc_id_ + "_" + std::to_string(ts) + ".png";

    // Try scrot first (most common)
    int ret = system(("scrot " + path + " 2>/dev/null").c_str());
    if (ret == 0) {
        std::ifstream check(path);
        if (check.good()) return path;
    }

    // Try import (ImageMagick)
    ret = system(("import -window root " + path + " 2>/dev/null").c_str());
    if (ret == 0) {
        std::ifstream check(path);
        if (check.good()) return path;
    }

    // Try gnome-screenshot
    ret = system(("gnome-screenshot -f " + path + " 2>/dev/null").c_str());
    if (ret == 0) {
        std::ifstream check(path);
        if (check.good()) return path;
    }

    return std::nullopt;
}

} // namespace nuub::infrastructure::media
