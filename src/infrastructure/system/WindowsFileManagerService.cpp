#include "infrastructure/system/WindowsFileManagerService.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace nuub::infrastructure::system {

namespace fs = std::filesystem;

std::string WindowsFileManagerService::list_directory(const std::string& path) {
    std::ostringstream oss;

    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return "Error: path not found: " + path;
    }

    oss << "Directory: " << path << "\n\n";

    for (const auto& entry : fs::directory_iterator(path, ec)) {
        auto file_size = fs::file_size(entry, ec);
        auto last_write = fs::last_write_time(entry, ec);

        std::string type = entry.is_directory() ? "[DIR] " : "[FILE]";
        std::string size = entry.is_directory() ? "-" : std::to_string(file_size) + " B";

        oss << type << " " << entry.path().filename().string()
            << "  (" << size << ")\n";
    }

    if (ec) {
        oss << "\nWarning: some entries could not be read\n";
    }

    return oss.str();
}

bool WindowsFileManagerService::create_directory(const std::string& path) {
    std::error_code ec;
    return fs::create_directories(path, ec);
}

bool WindowsFileManagerService::delete_file(const std::string& path) {
    std::error_code ec;
    return fs::remove(path, ec);
}

bool WindowsFileManagerService::delete_directory(const std::string& path) {
    std::error_code ec;
    return fs::remove_all(path, ec);
}

std::optional<std::string> WindowsFileManagerService::read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    // Telegram message limit
    constexpr size_t MAX_MSG = 4000;
    if (content.size() > MAX_MSG) {
        content.resize(MAX_MSG);
        content += "\n... (truncado)";
    }

    return content;
}

bool WindowsFileManagerService::write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(content.data(), content.size());
    return file.good();
}

bool WindowsFileManagerService::file_exists(const std::string& path) {
    std::error_code ec;
    return fs::exists(path, ec);
}

} // namespace nuub::infrastructure::system
