#pragma once

#include <string>
#include <unordered_set>

#include "domain/common/Types.hpp"

namespace nuub::domain::entities {

struct KeystrokeEntry {
    std::string key;
    TimePoint timestamp;
    std::unordered_set<std::string> modifiers;

    KeystrokeEntry(std::string k, TimePoint ts = now(),
                   std::unordered_set<std::string> mods = {})
        : key(std::move(k)), timestamp(ts), modifiers(std::move(mods)) {}
};

} // namespace nuub::domain::entities
