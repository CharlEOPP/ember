#include <catch2/catch_test_macros.hpp>
#include "ember/scripting/RegisterScript.h"
#include "ember/scripting/ScriptSystem.h"
#include "ember/scene/Scene.h"

#include <string>
#include <vector>

using namespace ember;

namespace {
std::vector<std::string> g_order;   // shared call log for ordering assertions

struct LT_Life : ScriptComponent {
    int starts = 0, updates = 0, lates = 0, fixeds = 0, destroys = 0;
    void onStart() override      { ++starts; }
    void onUpdate(f32) override  { ++updates; g_order.push_back("u"); }
    void onLateUpdate(f32) override { ++lates; g_order.push_back("l"); }
    void onFixedUpdate(f32) override { ++fixeds; }
    void onDestroy() override    { ++destroys; }
};
bool g_selfDestructDestroyed = false;
struct LT_SelfDestruct : ScriptComponent {
    bool started = false;
    int  updates = 0;
    void onStart() override     { started = true; }
    // onStart and the first onUpdate run on the same tick (Unity-style), so
    // destroy on the *second* update to observe a still-alive entity in between.
    void onUpdate(f32) override  { if (++updates >= 2) destroy(); }
    void onDestroy() override    { g_selfDestructDestroyed = true; }
};
struct LT_Boom : ScriptComponent {
    int updates = 0;
    void onUpdate(f32) override  { ++updates; throw std::runtime_error("boom"); }
};
struct LT_Quiet : ScriptComponent {
    int updates = 0;
    void onUpdate(f32) override  { ++updates; }
};
struct LT_A : ScriptComponent { void onUpdate(f32) override { g_order.push_back("A"); } };
struct LT_B : ScriptComponent { void onUpdate(f32) override { g_order.push_back("B"); } };

// Idempotent: registers the test scripts once for the whole TU.
struct Reg {
    Reg() {
        registerScriptTypeNoSerialize<LT_Life>("LT_Life");
        registerScriptTypeNoSerialize<LT_SelfDestruct>("LT_SelfDestruct");
        registerScriptTypeNoSerialize<LT_Boom>("LT_Boom");
        registerScriptTypeNoSerialize<LT_Quiet>("LT_Quiet");
        registerScriptTypeNoSerialize<LT_A>("LT_A", 10);   // later (higher order)
        registerScriptTypeNoSerialize<LT_B>("LT_B", 5);    // earlier (lower order)
    }
};
const Reg g_reg;
} // namespace

TEST_CASE("Script onStart fires once after emplace, before onUpdate", "[scripting]") {
    Scene sc("t"); ScriptSystem sys(sc);
    Entity e = sc.world().create();
    auto& s = sc.world().emplace<LT_Life>(e);
    REQUIRE(s.starts == 0);              // not before the first tick
    sys.update(sc.world(), 0.016f);
    REQUIRE(s.starts == 1);
    REQUIRE(s.updates == 1);
    sys.update(sc.world(), 0.016f);
    REQUIRE(s.starts == 1);             // not restarted
    REQUIRE(s.updates == 2);
}

TEST_CASE("Script onUpdate all run before onLateUpdate", "[scripting]") {
    g_order.clear();
    Scene sc("t"); ScriptSystem sys(sc);
    sc.world().emplace<LT_Life>(sc.world().create());
    sc.world().emplace<LT_Life>(sc.world().create());
    sys.update(sc.world(), 0.016f);
    // expect u,u,l,l
    REQUIRE(g_order.size() == 4);
    REQUIRE(g_order[0] == "u"); REQUIRE(g_order[1] == "u");
    REQUIRE(g_order[2] == "l"); REQUIRE(g_order[3] == "l");
}

TEST_CASE("Script onFixedUpdate uses a fixed accumulator", "[scripting]") {
    Scene sc("t"); ScriptSystem sys(sc); sys.setFixedDelta(0.02f);
    auto& s = sc.world().emplace<LT_Life>(sc.world().create());
    sys.update(sc.world(), 0.05f);     // 0.05 / 0.02 -> 2 steps, remainder 0.01
    REQUIRE(s.fixeds == 2);
    sys.update(sc.world(), 0.05f);     // 0.01 + 0.05 = 0.06 -> 3 more
    REQUIRE(s.fixeds == 5);
}

TEST_CASE("Script destroy() is deferred; onDestroy fires before removal", "[scripting]") {
    g_selfDestructDestroyed = false;
    Scene sc("t"); ScriptSystem sys(sc);
    Entity e = sc.world().create();
    auto& s = sc.world().emplace<LT_SelfDestruct>(e);
    sys.update(sc.world(), 0.016f);    // onStart + onUpdate #1 (no destroy yet)
    REQUIRE(s.started);
    REQUIRE(sc.world().valid(e));      // still alive between frames
    sys.update(sc.world(), 0.016f);    // onUpdate #2 -> destroy(); drained end of tick
    REQUIRE_FALSE(sc.world().valid(e));
    REQUIRE(g_selfDestructDestroyed);  // onDestroy fired before removal (SS-07)
}

TEST_CASE("Script exception is isolated and disables only that script", "[scripting]") {
    Scene sc("t"); ScriptSystem sys(sc);
    auto& boom  = sc.world().emplace<LT_Boom>(sc.world().create());
    auto& quiet = sc.world().emplace<LT_Quiet>(sc.world().create());
    sys.update(sc.world(), 0.016f);
    sys.update(sc.world(), 0.016f);
    REQUIRE(boom.updates == 1);        // threw once, then disabled
    REQUIRE(quiet.updates == 2);       // unaffected, no crash
}

TEST_CASE("Script execution order is (priority, name)", "[scripting]") {
    g_order.clear();
    Scene sc("t"); ScriptSystem sys(sc);
    sc.world().emplace<LT_A>(sc.world().create());   // order 10
    sc.world().emplace<LT_B>(sc.world().create());   // order 5
    sys.update(sc.world(), 0.016f);
    int ia = -1, ib = -1;
    for (int i = 0; i < (int)g_order.size(); ++i) {
        if (g_order[i] == "A" && ia < 0) ia = i;
        if (g_order[i] == "B" && ib < 0) ib = i;
    }
    REQUIRE(ib >= 0); REQUIRE(ia >= 0);
    REQUIRE(ib < ia);                  // B (order 5) before A (order 10)
}
