#pragma once

#include <string>
#include <unordered_set>

#include "domain/common/Types.hpp"

namespace nuub::domain::entities {

struct KeystrokeEntry {
    std::string key;
    std::string window_title;  // Active window when key was pressed
    TimePoint timestamp;
    std::unordered_set<std::string> modifiers;

    KeystrokeEntry(std::string k, TimePoint ts = now(),
                   std::unordered_set<std::string> mods = {},
                   std::string window = "")
        : key(std::move(k)), window_title(std::move(window)),
          timestamp(ts), modifiers(std::move(mods)) {}
};

} // namespace nuub::domain::entities
