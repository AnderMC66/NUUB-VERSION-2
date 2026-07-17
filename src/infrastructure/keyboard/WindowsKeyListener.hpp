#pragma once

#include <atomic>

#include <Windows.h>

#include "application/interfaces/IKeyListener.hpp"
#include "domain/services/IKeystrokeService.hpp"
#include "infrastructure/keyboard/KeyResolver.hpp"

namespace nuub::infrastructure::keyboard {

class WindowsKeyListener final : public application::interfaces::IKeyListener {
    domain::services::IKeystrokeService& service_;
    KeyResolver resolver_;
    HHOOK hook_ = nullptr;
    std::atomic<bool> running_{false};

    static LRESULT CALLBACK keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam);

public:
    explicit WindowsKeyListener(domain::services::IKeystrokeService& service);
    ~WindowsKeyListener() override;

    void start() override;
    void stop() override;
};

} // namespace nuub::infrastructure::keyboard
