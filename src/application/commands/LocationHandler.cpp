#include "application/commands/LocationHandler.hpp"

#include <algorithm>

namespace nuub::application::commands {

LocationHandler::LocationHandler(
    interfaces::IGeolocationService& geolocation,
    interfaces::IReporter& reporter,
    std::string pc_id)
    : geolocation_(geolocation)
    , reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool LocationHandler::matches(const std::string& target) const {
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

domain::Result<void> LocationHandler::handle_locate(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Obteniendo ubicacion...");
    auto info = geolocation_.get_location();

    std::string text = "*Ubicacion de " + pc_id_ + "*\n";
    text += "IP: `" + info["ip"] + "`\n";
    text += "Ciudad: " + info["city"] + "\n";
    text += "Region: " + info["region"] + "\n";
    text += "Pais: " + info["country"];

    if (info.count("maps_link")) {
        text += "\n[Google Maps](" + info["maps_link"] + ")";
    }

    reporter_.send_message(text);
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
