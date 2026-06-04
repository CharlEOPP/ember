#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ember/physics2d/Physics2DSystem.h"
#include "ember/physics2d/RigidBody2D.h"
#include "ember/physics2d/Colliders2D.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"

using namespace ember;
using Catch::Approx;

TEST_CASE("Dynamic body falls under gravity", "[physics2d]") {
    World w;
    Entity e = w.create();
    w.emplace<Transform>(e).position = {0.0f, 10.0f, 0.0f};
    w.emplace<RigidBody2D>(e);   // dynamic, gravityScale 1

    Physics2DSystem sys;
    sys.setGravity({0.0f, -10.0f});
    sys.step(w, 0.1f);

    const auto& rb = w.get<RigidBody2D>(e);
    REQUIRE(rb.velocity.y == Approx(-1.0f));            // -10 * 0.1
    REQUIRE(w.get<Transform>(e).position.y == Approx(9.9f));  // 10 + (-1)*0.1
}

TEST_CASE("Static body never moves", "[physics2d]") {
    World w;
    Entity e = w.create();
    w.emplace<Transform>(e).position = {3.0f, 3.0f, 0.0f};
    auto& rb = w.emplace<RigidBody2D>(e);
    rb.type = BodyType2D::Static;

    Physics2DSystem sys;
    for (int i = 0; i < 10; ++i) sys.step(w, 1.0f / 50.0f);
    REQUIRE(w.get<Transform>(e).position.y == Approx(3.0f));
}

TEST_CASE("Dynamic box comes to rest on a static floor", "[physics2d]") {
    World w;
    // Static floor: big box centered at y=0, half-height 0.5 (top at 0.5).
    Entity floor = w.create();
    w.emplace<Transform>(floor).position = {0.0f, 0.0f, 0.0f};
    w.emplace<RigidBody2D>(floor).type = BodyType2D::Static;
    w.emplace<BoxCollider2D>(floor).halfExtents = {10.0f, 0.5f};

    // Falling box: half-height 0.5, starts above.
    Entity box = w.create();
    w.emplace<Transform>(box).position = {0.0f, 5.0f, 0.0f};
    w.emplace<RigidBody2D>(box);
    w.emplace<BoxCollider2D>(box).halfExtents = {0.5f, 0.5f};

    Physics2DSystem sys;
    sys.setGravity({0.0f, -10.0f});
    for (int i = 0; i < 300; ++i) sys.step(w, 1.0f / 60.0f);

    // Rests with its bottom on the floor top (0.5): center ~1.0.
    REQUIRE(w.get<Transform>(box).position.y == Approx(1.0f).margin(0.05f));
}
