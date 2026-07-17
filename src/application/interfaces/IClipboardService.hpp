#pragma once

#include <string>

namespace nuub::application::interfaces {

class IClipboardService {
public:
    virtual ~IClipboardService() = default;

    [[nodiscard]] virtual std::string get_clipboard() = 0;
    virtual void set_clipboard(const std::string& text) = 0;
};

} // namespace nuub::application::interfaces
