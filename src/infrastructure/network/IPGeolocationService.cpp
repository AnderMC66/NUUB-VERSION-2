#include "infrastructure/network/IPGeolocationService.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace nuub::infrastructure::network {

using json = nlohmann::json;

static size_t write_cb(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

std::unordered_map<std::string, std::string> IPGeolocationService::get_location() {
    std::unordered_map<std::string, std::string> result;

    CURL* curl = curl_easy_init();
    if (!curl) {
        result["error"] = "CURL init failed";
        return result;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, API_URL);
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

        std::string loc = data.value("loc", "0,0");
        auto comma = loc.find(',');
        std::string lat = (comma != std::string::npos) ? loc.substr(0, comma) : "0";
        std::string lon = (comma != std::string::npos) ? loc.substr(comma + 1) : "0";

        result["ip"] = data.value("ip", "N/A");
        result["city"] = data.value("city", "N/A");
        result["region"] = data.value("region", "N/A");
        result["country"] = data.value("country", "N/A");
        result["maps_link"] = "https://www.google.com/maps/search/?api=1&query=" + lat + "," + lon;
    } catch (const std::exception& e) {
        result["error"] = e.what();
    }

    return result;
}

} // namespace nuub::infrastructure::network
