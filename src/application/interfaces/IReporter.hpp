#pragma once

#include <string>

namespace nuub::application::interfaces {

class IReporter {
public:
    virtual ~IReporter() = default;

    virtual void send_message(const std::string& text) = 0;
    virtual bool send_file(const std::string& path, const std::string& caption = "") = 0;
};

} // namespace nuub::application::interfaces
