#pragma once
// spdlog headers are included here so EMBER_LOG_* macros compile in any TU.
// The bundled stub in third_party/spdlog_stub/ is used when spdlog is unavailable.
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <string>

namespace ember {

class Log {
public:
    static void init();
    static void shutdown();
    static void setLevel(const std::string& channel, int level);

    static std::shared_ptr<spdlog::logger>& engineLogger();
    static std::shared_ptr<spdlog::logger>& gameLogger();

private:
    static std::shared_ptr<spdlog::logger> s_engineLogger;
    static std::shared_ptr<spdlog::logger> s_gameLogger;
};

} // namespace ember

// ---- Engine log macros ----
#define EMBER_LOG_TRACE(...)   ::ember::Log::engineLogger()->trace(__VA_ARGS__)
#define EMBER_LOG_DEBUG(...)   ::ember::Log::engineLogger()->debug(__VA_ARGS__)
#define EMBER_LOG_INFO(...)    ::ember::Log::engineLogger()->info(__VA_ARGS__)
#define EMBER_LOG_WARN(...)    ::ember::Log::engineLogger()->warn(__VA_ARGS__)
#define EMBER_LOG_ERROR(...)   ::ember::Log::engineLogger()->error(__VA_ARGS__)
#define EMBER_LOG_FATAL(...)   do { ::ember::Log::engineLogger()->critical(__VA_ARGS__); std::terminate(); } while(0)

// ---- Game log macros ----
#define GAME_LOG_TRACE(...)    ::ember::Log::gameLogger()->trace(__VA_ARGS__)
#define GAME_LOG_DEBUG(...)    ::ember::Log::gameLogger()->debug(__VA_ARGS__)
#define GAME_LOG_INFO(...)     ::ember::Log::gameLogger()->info(__VA_ARGS__)
#define GAME_LOG_WARN(...)     ::ember::Log::gameLogger()->warn(__VA_ARGS__)
#define GAME_LOG_ERROR(...)    ::ember::Log::gameLogger()->error(__VA_ARGS__)
#define GAME_LOG_FATAL(...)    do { ::ember::Log::gameLogger()->critical(__VA_ARGS__); std::terminate(); } while(0)

// Strip trace/debug in release
#if defined(NDEBUG)
#  undef  EMBER_LOG_TRACE
#  undef  EMBER_LOG_DEBUG
#  undef  GAME_LOG_TRACE
#  undef  GAME_LOG_DEBUG
#  define EMBER_LOG_TRACE(...) do {} while(0)
#  define EMBER_LOG_DEBUG(...) do {} while(0)
#  define GAME_LOG_TRACE(...)  do {} while(0)
#  define GAME_LOG_DEBUG(...)  do {} while(0)
#endif
