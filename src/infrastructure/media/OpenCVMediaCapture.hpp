#pragma once

#include <string>

#include "application/interfaces/IMediaCapture.hpp"

namespace nuub::infrastructure::media {

class OpenCVMediaCapture final : public application::interfaces::IMediaCapture {
    std::string pc_id_;

public:
    explicit OpenCVMediaCapture(std::string pc_id);

    [[nodiscard]] std::optional<std::string> take_photo() override;
    [[nodiscard]] std::optional<std::string> take_video(int duration_sec) override;
    [[nodiscard]] std::optional<std::string> record_audio(int duration_sec) override;
    [[nodiscard]] std::optional<std::string> take_screenshot() override;
    [[nodiscard]] std::optional<std::string> screen_record(int duration_sec) override;
};

} // namespace nuub::infrastructure::media
