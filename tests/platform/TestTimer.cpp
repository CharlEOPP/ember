#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>   // Catch::Approx lives in its own header in Catch2 v3
#include "ember/platform/Timer.h"
#include <thread>
#include <chrono>

using namespace ember;

TEST_CASE("Timer_ElapsedIncreases", "[timer]") {
    Timer t;
    const f64 first  = t.elapsed();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const f64 second = t.elapsed();
    REQUIRE(second > first);
}

TEST_CASE("Timer_ResetResetsElapsed", "[timer]") {
    Timer t;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    REQUIRE(t.elapsed() > 0.001);
    t.reset();
    REQUIRE(t.elapsed() < 0.002); // should be near-zero immediately after reset
}

TEST_CASE("Timer_SubMillisecondResolution", "[timer]") {
    Timer t;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const f64 ms = t.elapsed() * 1000.0;
    // Loose bounds — CI machines vary. Accept 0.5ms–50ms.
    REQUIRE(ms > 0.5);
    REQUIRE(ms < 50.0);
}

TEST_CASE("Time_UpdateAndQuery", "[timer]") {
    Time::update(0.016f); // simulate a 16ms frame
    REQUIRE(Time::deltaTime() == Catch::Approx(0.016f).epsilon(0.001f));
    REQUIRE(Time::elapsed()   > 0.0);
}
