#include "infrastructure/telegram/HeartbeatService.hpp"

#include <chrono>

namespace nuub::infrastructure::telegram {

HeartbeatService::HeartbeatService(
    std::function<void(const std::string&)> send_fn,
    std::string pc_id, int interval_minutes)
    : send_message_(std::move(send_fn))
    , pc_id_(std::move(pc_id))
    , interval_minutes_(interval_minutes) {}

void HeartbeatService::heartbeat_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::minutes(interval_minutes_));

        if (running_ && send_message_) {
            send_message_("[" + pc_id_ + "] Heartbeat - still alive");
        }
    }
}

void HeartbeatService::start() {
    if (running_) return;
    running_ = true;
    heartbeat_thread_ = std::thread(&HeartbeatService::heartbeat_loop, this);
}

void HeartbeatService::stop() {
    running_ = false;
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
}

} // namespace nuub::infrastructure::telegram
