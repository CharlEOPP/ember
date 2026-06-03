#include <catch2/catch_test_macros.hpp>
#include "ember/events/Signal.h"

using namespace ember;

namespace {
struct Ping { int value = 0; };
}

TEST_CASE("Signal_ImmediateInvoke", "[events]") {
    Signal<Ping> sig;
    int received = 0;
    sig.connect([&](const Ping& p) { received += p.value; });
    sig.connect([&](const Ping& p) { received += p.value; });

    REQUIRE(sig.size() == 2u);
    sig.fire(Ping{5});          // synchronous, immediate
    REQUIRE(received == 10);
}
