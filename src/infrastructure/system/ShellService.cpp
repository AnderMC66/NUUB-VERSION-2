#include "infrastructure/system/ShellService.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <string>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace nuub::infrastructure::system {

std::string ShellService::execute(const std::string& command) {
#ifdef _WIN32
    std::string full_cmd = "cmd.exe /c " + command + " 2>&1";
#else
    std::string full_cmd = command + " 2>&1";
#endif

    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen(full_cmd.c_str(), "r"), pclose);

    if (!pipe) return "Error: no se pudo ejecutar el comando.";

    std::array<char, 4096> buffer{};
    std::string result;

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    if (result.empty()) {
        result = "(sin salida)";
    }

    // Telegram message limit is 4096 characters
    constexpr size_t MAX_MSG = 4000;
    if (result.size() > MAX_MSG) {
        result.resize(MAX_MSG);
        result += "\n... (truncado)";
    }

    return result;
}

} // namespace nuub::infrastructure::system
