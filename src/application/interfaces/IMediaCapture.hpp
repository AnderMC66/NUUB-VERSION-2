#pragma once

#include <optional>
#include <string>

namespace nuub::application::interfaces {

class IMediaCapture {
public:
    virtual ~IMediaCapture() = default;

    [[nodiscard]] virtual std::optional<std::string> take_photo() = 0;
    [[nodiscard]] virtual std::optional<std::string> take_video(int duration_sec) = 0;
    [[nodiscard]] virtual std::optional<std::string> record_audio(int duration_sec) = 0;
    [[nodiscard]] virtual std::optional<std::string> take_screenshot() = 0;
    [[nodiscard]] virtual std::optional<std::string> screen_record(int duration_sec) = 0;
};

} // namespace nuub::application::interfaces
