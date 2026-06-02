#include "ember/core/Log.h"

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <filesystem>
#include <vector>

namespace ember {

std::shared_ptr<spdlog::logger> Log::s_engineLogger;
std::shared_ptr<spdlog::logger> Log::s_gameLogger;

void Log::init() {
    if (s_engineLogger) return; // idempotent

    // Ensure logs/ directory exists
    std::filesystem::create_directories("logs");

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/ember.log",
        1024 * 1024 * 5, // 5 MB per file
        3                // 3 rotations
    ));

    const char* pattern = "[%H:%M:%S.%e] [%n] [%^%l%$] %v";

    s_engineLogger = std::make_shared<spdlog::logger>("EMBER", sinks.begin(), sinks.end());
    s_engineLogger->set_level(spdlog::level::trace);
    s_engineLogger->set_pattern(pattern);
    s_engineLogger->flush_on(spdlog::level::warn);

    s_gameLogger = std::make_shared<spdlog::logger>("GAME", sinks.begin(), sinks.end());
    s_gameLogger->set_level(spdlog::level::trace);
    s_gameLogger->set_pattern(pattern);
    s_gameLogger->flush_on(spdlog::level::warn);

    spdlog::register_logger(s_engineLogger);
    spdlog::register_logger(s_gameLogger);
}

void Log::shutdown() {
    if (s_engineLogger) s_engineLogger->flush();
    if (s_gameLogger)   s_gameLogger->flush();
    spdlog::shutdown();
    s_engineLogger.reset();
    s_gameLogger.reset();
}

void Log::setLevel(const std::string& channel, int level) {
    if (auto logger = spdlog::get(channel))
        logger->set_level(static_cast<spdlog::level::level_enum>(level));
}

std::shared_ptr<spdlog::logger>& Log::engineLogger() { return s_engineLogger; }
std::shared_ptr<spdlog::logger>& Log::gameLogger()   { return s_gameLogger;   }

} // namespace ember
