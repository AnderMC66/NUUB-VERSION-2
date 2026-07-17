#include "infrastructure/keyboard/LinuxKeyListener.hpp"

#include <cstdio>
#include <fstream>
#include <string>

namespace nuub::infrastructure::keyboard {

LinuxKeyListener::LinuxKeyListener(domain::services::IKeystrokeService& service)
    : service_(service) {}

LinuxKeyListener::~LinuxKeyListener() {
    stop();
}

void LinuxKeyListener::listen_loop() {
    // Try to open /dev/input/event0 for keyboard events
    // Falls back to no-op if not available (requires root)
    std::ifstream input("/dev/input/event0", std::ios::binary);
    if (!input.is_open()) {
        fprintf(stderr, "[LinuxKeyListener] /dev/input/event0 not available. "
                        "Keyboard logging requires root access.\n");
        return;
    }

    // Simplified event structure for evdev
    struct input_event {
        unsigned long long time;
        unsigned short type;
        unsigned short code;
        int value;
    };

    while (running_) {
        input_event event{};
        input.read(reinterpret_cast<char*>(&event), sizeof(event));
        if (!input) break;

        // EV_KEY = 0x01, value 1 = press, 0 = release
        if (event.type == 0x01 && event.value == 1) {
            // Simple ASCII mapping for common keys
            std::string key;
            if (event.code >= 2 && event.code <= 13) {
                // Number row: 1-9, 0, -, =, backspace
                const char* keys = "1234567890-=";
                key = std::string(1, keys[event.code - 2]);
            } else if (event.code >= 16 && event.code <= 25) {
                // QWERTY row
                const char* keys = "qwertyuiop";
                key = std::string(1, keys[event.code - 16]);
            } else if (event.code >= 30 && event.code <= 38) {
                // ASDF row
                const char* keys = "asdfghjkl";
                key = std::string(1, keys[event.code - 30]);
            } else if (event.code >= 44 && event.code <= 53) {
                // ZXCV row
                const char* keys = "zxcvbnm,./";
                key = std::string(1, keys[event.code - 44]);
            } else if (event.code == 57) {
                key = " ";
            } else if (event.code == 28) {
                key = "\n";
            }

            if (!key.empty()) {
                service_.process_press(key);
            }
        }
    }
}

void LinuxKeyListener::start() {
    if (running_) return;
    running_ = true;
    listener_thread_ = std::thread(&LinuxKeyListener::listen_loop, this);
}

void LinuxKeyListener::stop() {
    running_ = false;
    if (listener_thread_.joinable()) {
        listener_thread_.join();
    }
}

} // namespace nuub::infrastructure::keyboard
