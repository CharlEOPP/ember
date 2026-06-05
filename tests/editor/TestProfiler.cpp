// Force the scope macro on for this TU regardless of the build option.
#define EMBER_ENABLE_PROFILING
#include <catch2/catch_test_macros.hpp>
#include "ember/core/Profiler.h"

using namespace ember;

TEST_CASE("Profiler records nested scopes with correct depth", "[editor][profiler]") {
    Profiler& p = Profiler::instance();
    p.setEnabled(true);
    p.beginFrame();
    {
        EMBER_PROFILE_SCOPE("outer");
        { EMBER_PROFILE_SCOPE("innerA"); }
        { EMBER_PROFILE_SCOPE("innerB"); }
    }
    p.endFrame();

    const auto& f = p.lastFrame();
    REQUIRE(f.size() == 3);
    REQUIRE(f[0].name == "outer");  REQUIRE(f[0].depth == 0);
    REQUIRE(f[1].name == "innerA"); REQUIRE(f[1].depth == 1);
    REQUIRE(f[2].name == "innerB"); REQUIRE(f[2].depth == 1);
    for (const auto& s : f) REQUIRE(s.end >= s.start);
}

TEST_CASE("Profiler records nothing when disabled", "[editor][profiler]") {
    Profiler& p = Profiler::instance();
    p.setEnabled(false);
    p.beginFrame();
    { EMBER_PROFILE_SCOPE("x"); }
    p.endFrame();
    REQUIRE(p.lastFrame().empty());
    p.setEnabled(true);   // restore for other tests
}
