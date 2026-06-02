#pragma once
#include "../logger.h"
#include <string>
#include <cstddef>
namespace spdlog::sinks {
struct rotating_file_sink_mt : spdlog::sink {
    rotating_file_sink_mt(const std::string&, std::size_t, int) {}
};
} // namespace spdlog::sinks
