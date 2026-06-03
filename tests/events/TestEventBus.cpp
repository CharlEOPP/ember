#include <catch2/catch_test_macros.hpp>
#include "ember/events/EventBus.h"
#include "ember/events/Events.h"

using namespace ember;

TEST_CASE("EventBus_PostDispatchDelivers", "[events]") {
    EventBus bus;
    u32 gotW = 0, gotH = 0;
    auto tok = bus.subscribe<WindowResizeEvent>([&](const WindowResizeEvent& e) {
        gotW = e.width; gotH = e.height;
    });
    bus.post<WindowResizeEvent>(1920u, 1080u);
    bus.dispatch();
    REQUIRE(gotW == 1920u);
    REQUIRE(gotH == 1080u);
}

TEST_CASE("EventBus_DeferredNotImmediate", "[events]") {
    EventBus bus;
    int calls = 0;
    auto tok = bus.subscribe<WindowCloseEvent>([&](const WindowCloseEvent&) { ++calls; });
    bus.post<WindowCloseEvent>();
    REQUIRE(calls == 0);   // not delivered until dispatch()
    bus.dispatch();
    REQUIRE(calls == 1);
}

TEST_CASE("EventBus_TokenAutoUnsubscribe", "[events]") {
    EventBus bus;
    int calls = 0;
    {
        auto tok = bus.subscribe<WindowCloseEvent>([&](const WindowCloseEvent&) { ++calls; });
        bus.post<WindowCloseEvent>();
        bus.dispatch();
        REQUIRE(calls == 1);
    } // token destroyed here -> unsubscribed
    bus.post<WindowCloseEvent>();
    bus.dispatch();
    REQUIRE(calls == 1);   // no further delivery
}

TEST_CASE("EventBus_MultipleSubscribers", "[events]") {
    EventBus bus;
    int order = 0, a = 0, b = 0;
    auto t1 = bus.subscribe<WindowCloseEvent>([&](const WindowCloseEvent&) { a = ++order; });
    auto t2 = bus.subscribe<WindowCloseEvent>([&](const WindowCloseEvent&) { b = ++order; });
    bus.post<WindowCloseEvent>();
    bus.dispatch();
    REQUIRE(a == 1);       // subscription order preserved
    REQUIRE(b == 2);
}

TEST_CASE("EventBus_RingBufferCapacity", "[events]") {
    EventBus bus;
    bus.reserve<MouseMovedEvent>(4);
    int delivered = 0;
    auto tok = bus.subscribe<MouseMovedEvent>([&](const MouseMovedEvent&) { ++delivered; });
    for (int i = 0; i < 10; ++i) bus.post<MouseMovedEvent>();  // overflow logs a warning
    bus.dispatch();
    REQUIRE(delivered == 4);   // capped at capacity, no crash
}
