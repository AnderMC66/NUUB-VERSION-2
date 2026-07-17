#pragma once

#include <atomic>
#include <thread>

#include "application/interfaces/IKeyListener.hpp"
#include "domain/services/IKeystrokeService.hpp"

namespace nuub::infrastructure::keyboard {

class LinuxKeyListener final : public application::interfaces::IKeyListener {
    domain::services::IKeystrokeService& service_;
    std::atomic<bool> running_{false};
    std::thread listener_thread_;

    void listen_loop();

public:
    explicit LinuxKeyListener(domain::services::IKeystrokeService& service);
    ~LinuxKeyListener() override;

    void start() override;
    void stop() override;
};

} // namespace nuub::infrastructure::keyboard
