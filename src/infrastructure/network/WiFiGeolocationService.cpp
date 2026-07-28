#include "infrastructure/network/WiFiGeolocationService.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "domain/common/StringTable.hpp"

namespace nuub::infrastructure::network {

using json = nlohmann::json;

static size_t write_cb(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

WiFiGeolocationService::WiFiGeolocationService(
    application::interfaces::IWifiService& wifi,
    std::string google_api_key)
    : wifi_(wifi)
    , google_api_key_(std::move(google_api_key)) {}

// ── Apple Wi-Fi Positioning Service (via wifitracker.fun proxy) ─────────
// Uses Apple's WPS database. No API key needed. ~15-30m accuracy.
std::unordered_map<std::string, std::string> WiFiGeolocationService::apple_wps_location(
    const std::vector<application::interfaces::WifiAccessPoint>& aps) {

    std::unordered_map<std::string, std::string> result;

    std::string url = domain::StringTable::get("apple_wps");
    json payload;

    if (aps.size() == 1) {
        payload["bssid"] = aps[0].bssid;
    } else {
        json access_points = json::array();
        for (const auto& ap : aps) {
            json j;
            j["bssid"] = ap.bssid;
            // Convert percent to approximate dBm
            j["signal"] = (ap.signal_percent / 2) - 100;
            access_points.push_back(std::move(j));
        }
        payload["accessPoints"] = std::move(access_points);
    }

    std::string payload_str = payload.dump();

    CURL* curl = curl_easy_init();
    if (!curl) {
        result["error"] = "CURL init failed";
        return result;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload_str.size()));

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        result["error"] = curl_easy_strerror(res);
        return result;
    }

    try {
        auto data = json::parse(response);

        if (!data.value("success", false)) {
            return result;
        }

        auto loc = data["location"];
        double lat = loc.value("lat", 0.0);
        double lon = loc.value("lon", 0.0);
        int hacc = loc.value("hacc", 0);

        result["lat"] = std::to_string(lat);
        result["lon"] = std::to_string(lon);
        result["accuracy"] = std::to_string(hacc) + "m";
        result["maps_link"] = domain::StringTable::get("maps_link") +
            std::to_string(lat) + "," + std::to_string(lon);
        result["source"] = "apple_wps";

        // Try to get city/region/country from IP fallback as supplement
        auto ip_info = fallback_ip_location();
        result["ip"] = ip_info.count("ip") ? ip_info["ip"] : "N/A";
        result["city"] = ip_info.count("city") ? ip_info["city"] : "N/A";
        result["region"] = ip_info.count("region") ? ip_info["region"] : "N/A";
        result["country"] = ip_info.count("country") ? ip_info["country"] : "N/A";

    } catch (const std::exception& e) {
        result["error"] = e.what();
    }

    return result;
}

// ── Windows Native Location (via PowerShell → System.Device.Location) ───
std::unordered_map<std::string, std::string> WiFiGeolocationService::windows_native_location() {
    std::unordered_map<std::string, std::string> result;

#ifdef _WIN32
    const char* ps_cmd =
        "powershell -NoProfile -NonInteractive -Command \""
        "Add-Type -AssemblyName System.Device; "
        "$w=New-Object System.Device.Location.GeoCoordinateWatcher; "
        "$w.TryStart($true,[TimeSpan]::FromSeconds(15)) | Out-Null; "
        "if($w.Position.Location.IsUnknown){'unknown'}else{"
        "$w.Position.Location.Latitude.ToString()+'|'+"
        "$w.Position.Location.Longitude.ToString()+'|'+"
        "$w.Position.Location.HorizontalAccuracy.ToString()"
        "}\" 2>NUL";

    FILE* pipe = _popen(ps_cmd, "r");
    if (!pipe) return result;

    std::array<char, 512> buf{};
    std::string output;
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        output += buf.data();
    }
    int exit_code = _pclose(pipe);

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r' || output.back() == ' ')) {
        output.pop_back();
    }

    if (output == "unknown" || output.empty() || exit_code != 0) {
        return result;
    }

    auto first = output.find('|');
    auto second = output.rfind('|');
    if (first != std::string::npos && second != std::string::npos && first != second) {
        result["lat"] = output.substr(0, first);
        result["lon"] = output.substr(first + 1, second - first - 1);
        result["accuracy"] = output.substr(second + 1) + "m";
        result["source"] = "windows_location";
        result["maps_link"] = domain::StringTable::get("maps_link") +
            result["lat"] + "," + result["lon"];
    }
#endif

    return result;
}

// ── Google WiFi Geolocation API (requires API key) ─────────────────────
std::unordered_map<std::string, std::string> WiFiGeolocationService::google_wifi_location(
    const std::vector<application::interfaces::WifiAccessPoint>& aps) {

    std::unordered_map<std::string, std::string> result;

    std::string url = domain::StringTable::get("google_geo") + google_api_key_;

    json payload;
    payload["considerIp"] = true;
    json wifi_aps = json::array();
    for (const auto& ap : aps) {
        json j;
        j["macAddress"] = ap.bssid;
        j["signalStrength"] = (ap.signal_percent / 2) - 100;
        wifi_aps.push_back(std::move(j));
    }
    payload["wifiAccessPoints"] = std::move(wifi_aps);
    std::string payload_str = payload.dump();

    CURL* curl = curl_easy_init();
    if (!curl) {
        result["error"] = "CURL init failed";
        return result;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload_str.size()));

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        result["error"] = curl_easy_strerror(res);
        return result;
    }

    try {
        auto data = json::parse(response);

        if (data.contains("error")) {
            result["error"] = data["error"]["message"].get<std::string>();
            return result;
        }

        double lat = data.value("location", json::object()).value("lat", 0.0);
        double lng = data.value("location", json::object()).value("lng", 0.0);
        double accuracy = data.value("accuracy", 0.0);

        result["lat"] = std::to_string(lat);
        result["lon"] = std::to_string(lng);
        result["accuracy"] = std::to_string(static_cast<int>(accuracy)) + "m";
        result["maps_link"] = domain::StringTable::get("maps_link") +
            std::to_string(lat) + "," + std::to_string(lng);
        result["source"] = "wifi_google";

        auto ip_loc = fallback_ip_location();
        result["ip"] = ip_loc.count("ip") ? ip_loc["ip"] : "N/A";
        result["city"] = ip_loc.count("city") ? ip_loc["city"] : "N/A";
        result["region"] = ip_loc.count("region") ? ip_loc["region"] : "N/A";
        result["country"] = ip_loc.count("country") ? ip_loc["country"] : "N/A";

    } catch (const std::exception& e) {
        result["error"] = e.what();
    }

    return result;
}

// ── IP-based fallback (ip-api.com) ─────────────────────────────────────
std::unordered_map<std::string, std::string> WiFiGeolocationService::fallback_ip_location() {
    std::unordered_map<std::string, std::string> result;

    CURL* curl = curl_easy_init();
    if (!curl) {
        result["error"] = "CURL init failed";
        return result;
    }

    std::string ipapi_url = domain::StringTable::get("ipapi");
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, ipapi_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        result["error"] = curl_easy_strerror(res);
        return result;
    }

    try {
        auto data = json::parse(response);

        result["ip"] = data.value("query", "N/A");
        result["city"] = data.value("city", "N/A");
        result["region"] = data.value("regionName", "N/A");
        result["country"] = data.value("country", "N/A");
        result["lat"] = std::to_string(data.value("lat", 0.0));
        result["lon"] = std::to_string(data.value("lon", 0.0));
        result["source"] = "ip";
        result["accuracy"] = "~1000m";
        result["maps_link"] = domain::StringTable::get("maps_link") +
            result["lat"] + "," + result["lon"];
    } catch (const std::exception& e) {
        result["error"] = e.what();
    }

    return result;
}

// ── Main entry point ──────────────────────────────────────────────────
std::unordered_map<std::string, std::string> WiFiGeolocationService::get_location() {
    // Priority 1: Apple WPS (WiFi fingerprint, 15-30m accuracy, no key)
    auto aps = wifi_.get_visible_networks();
    if (!aps.empty()) {
        auto apple = apple_wps_location(aps);
        if (apple.count("lat")) {
            return apple;
        }
    }

    // Priority 2: Windows native location (GPS/WiFi, no key)
    auto native = windows_native_location();
    if (native.count("lat")) {
        auto ip_info = fallback_ip_location();
        native["ip"] = ip_info.count("ip") ? ip_info["ip"] : "N/A";
        native["city"] = ip_info.count("city") ? ip_info["city"] : "N/A";
        native["region"] = ip_info.count("region") ? ip_info["region"] : "N/A";
        native["country"] = ip_info.count("country") ? ip_info["country"] : "N/A";
        return native;
    }

    // Priority 3: Google WiFi Geolocation (requires API key)
    if (!google_api_key_.empty() && !aps.empty()) {
        auto google = google_wifi_location(aps);
        if (google.count("lat")) {
            return google;
        }
    }

    // Priority 4: IP-based fallback (city-level, ~1km accuracy)
    auto result = fallback_ip_location();
    return result;
}

} // namespace nuub::infrastructure::network