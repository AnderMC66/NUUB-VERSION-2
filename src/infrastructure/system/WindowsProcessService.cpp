#include "infrastructure/system/WindowsProcessService.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <sstream>

#include "domain/common/StringTable.hpp"

namespace nuub::infrastructure::system {

std::string WindowsProcessService::list_processes() {
    // Use encrypted string for tasklist command
    std::string tasklist = domain::StringTable::get("tasklist");
    FILE* pipe = _popen(tasklist.c_str(), "r");
    if (!pipe) return "Error: could not execute tasklist";

    std::string result;
    std::array<char, 4096> buffer{};

    // Skip CSV header
    if (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
        result = "PID\tName\n---\t----\n";
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        std::string line(buffer.data());
        // Parse CSV: "Image Name","PID","Session","Session#","Mem Usage"
        auto first_quote = line.find('"');
        if (first_quote == std::string::npos) continue;
        auto second_quote = line.find('"', first_quote + 1);
        if (second_quote == std::string::npos) continue;
        auto third_quote = line.find('"', second_quote + 1);
        if (third_quote == std::string::npos) continue;
        auto fourth_quote = line.find('"', third_quote + 1);
        if (fourth_quote == std::string::npos) continue;

        std::string name = line.substr(first_quote + 1, second_quote - first_quote - 1);
        std::string pid = line.substr(third_quote + 1, fourth_quote - third_quote - 1);

        result += pid + "\t" + name + "\n";
    }

    _pclose(pipe);
    return result;
}

bool WindowsProcessService::kill_process(int pid) {
    // Use encrypted string for taskkill command
    std::string taskkill = domain::StringTable::get("taskkill");
    std::string cmd = taskkill + std::to_string(pid) + " /F 2>&1";
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return false;

    std::array<char, 4096> buffer{};
    std::string output;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    int ret = _pclose(pipe);
    return ret == 0;
}

} // namespace nuub::infrastructure::system
