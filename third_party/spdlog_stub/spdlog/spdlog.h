#pragma once
#include "logger.h"
#include <unordered_map>

namespace spdlog {

inline std::unordered_map<std::string, std::shared_ptr<logger>>& _registry() {
    static std::unordered_map<std::string, std::shared_ptr<logger>> r;
    return r;
}

inline void register_logger(std::shared_ptr<logger> l) {
    if (l) _registry()[l->name()] = l;
}

inline std::shared_ptr<logger> get(const std::string& name) {
    auto it = _registry().find(name);
    return it != _registry().end() ? it->second : nullptr;
}

inline void shutdown() { _registry().clear(); }

} // namespace spdlog
