#include "infrastructure/system/WindowsShellService.hpp"

#include <array>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <cctype>

#include "domain/common/StringTable.hpp"

namespace nuub::infrastructure::system {

namespace {

// Sanitize shell command to prevent injection attacks.
// Blocks command chaining, pipe, redirection, and subshells.
std::string sanitize_command(const std::string& command) {
    // Check for dangerous shell operators that enable injection
    static const std::vector<std::string> blocked = {
        "|", "||", "&&", "&",
        ">", ">>", "<",
        "(", ")",
        ";",
        "`",
        "%",  // environment variable expansion in cmd
    };

    for (const auto& op : blocked) {
        if (command.find(op) != std::string::npos) {
            return ""; // reject
        }
    }

    // Block backslash-n, backslash-t (newline/tab injection)
    for (size_t i = 0; i + 1 < command.size(); ++i) {
        if (command[i] == '\\') {
            char next = command[i + 1];
            if (next == 'n' || next == 't' || next == 'r') {
                return ""; // reject
            }
        }
    }

    return command;
}

} // anonymous namespace

std::string WindowsShellService::execute(const std::string& command) {
    std::string sanitized = sanitize_command(command);
    if (sanitized.empty()) {
        return "Error: comando bloqueado por seguridad.";
    }

    // Use encrypted string for cmd.exe
    std::string cmd_exe = domain::StringTable::get("cmd_exe");
    std::string full_cmd = cmd_exe + " /c " + sanitized + " 2>&1";

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
