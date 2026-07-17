#pragma once

#include <functional>

namespace nuub::application::interfaces {

class IPersistenceService {
public:
    virtual ~IPersistenceService() = default;

    virtual void configure_auto_start() = 0;
    virtual void hide_console() = 0;
    virtual void start_anti_sleep() = 0;
    virtual void stop_anti_sleep() = 0;
    virtual void create_hidden_window(std::function<void()> on_shutdown) = 0;
    virtual void pump_messages() = 0;
};

} // namespace nuub::application::interfaces
