#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace nuub::infrastructure::telegram {

class HeartbeatService {
    std::function<void(const std::string&)> send_message_;
    std::string pc_id_;
    int interval_minutes_;
    std::atomic<bool> running_{false};
    std::thread heartbeat_thread_;

    void heartbeat_loop();

public:
    HeartbeatService(std::function<void(const std::string&)> send_fn,
                     std::string pc_id, int interval_minutes = 30);

    void start();
    void stop();
};

} // namespace nuub::infrastructure::telegram
