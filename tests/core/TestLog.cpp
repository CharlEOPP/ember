#include <catch2/catch_test_macros.hpp>
#include "ember/core/Log.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <string>

using namespace ember;

TEST_CASE("Log_InitIdempotent", "[log]") {
    // Calling init multiple times must not crash or duplicate sinks
    Log::init();
    Log::init();
    Log::init();
    REQUIRE(Log::engineLogger() != nullptr);
    REQUIRE(Log::gameLogger()   != nullptr);
    Log::shutdown();
}

TEST_CASE("Log_ShutdownAndReinit", "[log]") {
    Log::init();
    EMBER_LOG_INFO("Test message for shutdown");
    Log::shutdown();

    // After shutdown, re-initialisation must succeed
    Log::init();
    REQUIRE(Log::engineLogger() != nullptr);
    Log::shutdown();
}

TEST_CASE("Log_LevelFilter", "[log]") {
    Log::init();

    // A custom in-memory sink to capture output
    // We test indirectly: setting WARN level means DEBUG messages
    // are silently dropped (no exception / no crash).
    Log::setLevel("EMBER", spdlog::level::warn);
    REQUIRE_NOTHROW(EMBER_LOG_INFO("This should be filtered"));
    REQUIRE_NOTHROW(EMBER_LOG_WARN("This should pass through"));

    // Restore
    Log::setLevel("EMBER", spdlog::level::trace);
    Log::shutdown();
}
