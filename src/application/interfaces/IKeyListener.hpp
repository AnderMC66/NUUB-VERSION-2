#pragma once

#include <functional>

namespace nuub::application::interfaces {

class IKeyListener {
public:
    virtual ~IKeyListener() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
};

} // namespace nuub::application::interfaces
