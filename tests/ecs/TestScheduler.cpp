#include <catch2/catch_test_macros.hpp>
#include "ember/ecs/SystemScheduler.h"

#include <string>
#include <vector>

using namespace ember;

namespace {
std::vector<std::string> g_order;

struct SysA : ISystem {
    void update(World&, f32 dt) override { g_order.push_back("A:" + std::to_string(dt)); }
};
struct SysB : ISystem {
    void update(World&, f32) override { g_order.push_back("B"); }
};
struct SysPre : ISystem {
    void update(World&, f32) override { g_order.push_back("PRE"); }
};
} // namespace

TEST_CASE("Scheduler_PhaseOrder", "[ecs]") {
    g_order.clear();
    SystemScheduler sched;
    World world;

    // Register out of phase order; A before B within Update.
    sched.addSystem<SysA>(Phase::Update);
    sched.addSystem<SysB>(Phase::Update);
    sched.addSystem<SysPre>(Phase::PreUpdate);

    sched.runPhase(Phase::PreUpdate, world, 0.0f);
    sched.runPhase(Phase::Update,    world, 0.5f);

    REQUIRE(g_order.size() == 3u);
    REQUIRE(g_order[0] == "PRE");        // PreUpdate ran first (we called it first)
    REQUIRE(g_order[1] == "A:0.500000"); // registration order within phase + dt forwarded
    REQUIRE(g_order[2] == "B");
}
