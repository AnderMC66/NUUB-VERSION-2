#pragma once

#include <optional>
#include <string>

#include "application/interfaces/IMediaCapture.hpp"

namespace nuub::tests::mocks {

class MockMediaCapture final : public application::interfaces::IMediaCapture {
public:
    bool fail_photo = false;
    bool fail_video = false;
    bool fail_audio = false;
    bool fail_screenshot = false;
    bool fail_screenrecord = false;

    [[nodiscard]] std::optional<std::string> take_photo() override {
        return fail_photo ? std::nullopt : std::make_optional<std::string>("test_photo.jpg");
    }

    [[nodiscard]] std::optional<std::string> take_video(int) override {
        return fail_video ? std::nullopt : std::make_optional<std::string>("test_video.mp4");
    }

    [[nodiscard]] std::optional<std::string> record_audio(int) override {
        return fail_audio ? std::nullopt : std::make_optional<std::string>("test_audio.wav");
    }

    [[nodiscard]] std::optional<std::string> take_screenshot() override {
        return fail_screenshot ? std::nullopt : std::make_optional<std::string>("test_screenshot.png");
    }

    [[nodiscard]] std::optional<std::string> screen_record(int) override {
        return fail_screenrecord ? std::nullopt : std::make_optional<std::string>("test_screenrecord.mp4");
    }
};

} // namespace nuub::tests::mocks
