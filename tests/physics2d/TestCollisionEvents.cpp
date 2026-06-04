#include <catch2/catch_test_macros.hpp>
#include "ember/scripting/RegisterScript.h"
#include "ember/scripting/ScriptSystem.h"
#include "ember/physics2d/Physics2DSystem.h"
#include "ember/physics2d/RigidBody2D.h"
#include "ember/physics2d/Colliders2D.h"
#include "ember/scene/Scene.h"
#include "ember/ecs/Components.h"

using namespace ember;

namespace {
struct CE_Counter : ScriptComponent {
    int enter = 0, stay = 0, exit = 0, tEnter = 0, tExit = 0;
    void onCollisionEnter(Entity) override { ++enter; }
    void onCollisionStay(Entity)  override { ++stay; }
    void onCollisionExit(Entity)  override { ++exit; }
    void onTriggerEnter(Entity)   override { ++tEnter; }
    void onTriggerExit(Entity)    override { ++tExit; }
};
struct Reg { Reg() { registerScriptTypeNoSerialize<CE_Counter>("CE_Counter"); } };
const Reg g_reg;

// Static box at `x` with the counter script; half-extent 0.5.
Entity counterBox(Scene& sc, float x) {
    Entity e = sc.world().create();
    sc.world().emplace<Transform>(e).position = {x, 0.0f, 0.0f};
    sc.world().emplace<BoxCollider2D>(e).halfExtents = {0.5f, 0.5f};
    sc.world().emplace<CE_Counter>(e);
    return e;
}
// Kinematic box moving with `vx`, optionally a trigger.
Entity moverBox(Scene& sc, float x, float vx, bool trigger) {
    Entity e = sc.world().create();
    sc.world().emplace<Transform>(e).position = {x, 0.0f, 0.0f};
    auto& rb = sc.world().emplace<RigidBody2D>(e); rb.type = BodyType2D::Kinematic; rb.velocity = {vx, 0.0f};
    auto& col = sc.world().emplace<BoxCollider2D>(e); col.halfExtents = {0.5f, 0.5f}; col.isTrigger = trigger;
    return e;
}
} // namespace

TEST_CASE("Collision enter/stay/exit fire across a pass-through", "[physics2d]") {
    Scene sc("t");
    ScriptSystem scripts(sc);
    Physics2DSystem phys;
    phys.setGravity({0, 0});
    phys.setScriptSystem(&scripts);

    Entity a = counterBox(sc, 0.0f);
    moverBox(sc, 2.0f, -1.0f, /*trigger*/false);   // sweeps left through A

    for (int i = 0; i < 200; ++i) phys.step(sc.world(), 1.0f / 50.0f);

    const auto& cc = sc.world().get<CE_Counter>(a);
    REQUIRE(cc.enter == 1);     // exactly one begin
    REQUIRE(cc.stay  >= 1);     // overlapping for several steps
    REQUIRE(cc.exit  == 1);     // one end
    REQUIRE(cc.tEnter == 0);    // it was a solid collision, not a trigger
}

TEST_CASE("Trigger overlap fires trigger events only", "[physics2d]") {
    Scene sc("t");
    ScriptSystem scripts(sc);
    Physics2DSystem phys;
    phys.setGravity({0, 0});
    phys.setScriptSystem(&scripts);

    Entity a = counterBox(sc, 0.0f);
    Entity t = moverBox(sc, 2.0f, -1.0f, /*trigger*/true);

    for (int i = 0; i < 200; ++i) phys.step(sc.world(), 1.0f / 50.0f);

    const auto& cc = sc.world().get<CE_Counter>(a);
    REQUIRE(cc.tEnter == 1);
    REQUIRE(cc.tExit  == 1);
    REQUIRE(cc.enter  == 0);    // no solid-collision callbacks for a trigger
    // No physical response: the static counter box never moved.
    REQUIRE(sc.world().get<Transform>(a).position.x == 0.0f);
    (void)t;
}
