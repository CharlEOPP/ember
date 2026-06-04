#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ember/physics2d/Physics2DSystem.h"
#include "ember/physics2d/RigidBody2D.h"
#include "ember/physics2d/Colliders2D.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"

#include <cmath>

using namespace ember;
using Catch::Approx;

TEST_CASE("Overlapping dynamic boxes push apart", "[physics2d]") {
    World w;
    auto makeBox = [&](glm::vec2 pos) {
        Entity e = w.create();
        w.emplace<Transform>(e).position = {pos.x, pos.y, 0.0f};
        auto& rb = w.emplace<RigidBody2D>(e); rb.gravityScale = 0.0f;
        w.emplace<BoxCollider2D>(e).halfExtents = {0.5f, 0.5f};
        return e;
    };
    Entity a = makeBox({0.0f, 0.0f});
    Entity b = makeBox({0.3f, 0.0f});   // overlapping (centers 0.3 < 1.0 apart)

    Physics2DSystem sys;
    sys.setGravity({0.0f, 0.0f});
    for (int i = 0; i < 60; ++i) sys.step(w, 1.0f / 60.0f);

    const f32 dx = std::abs(w.get<Transform>(b).position.x - w.get<Transform>(a).position.x);
    REQUIRE(dx >= Approx(1.0f).margin(0.05f));   // separated to ~sum of half-extents
}

TEST_CASE("Raycast hits the nearest box", "[physics2d]") {
    World w;
    Entity e = w.create();
    w.emplace<Transform>(e).position = {0.0f, 0.0f, 0.0f};
    w.emplace<BoxCollider2D>(e).halfExtents = {1.0f, 1.0f};   // static (no body)

    Physics2DSystem sys;
    RaycastHit2D hit = sys.raycast(w, {-5.0f, 0.0f}, {1.0f, 0.0f}, 100.0f);
    REQUIRE(hit.hit);
    REQUIRE(hit.entity == e);
    REQUIRE(hit.point.x == Approx(-1.0f));          // left face of the box
    REQUIRE(hit.distance == Approx(4.0f));
}

TEST_CASE("Overlap queries report colliders", "[physics2d]") {
    World w;
    Entity e = w.create();
    w.emplace<Transform>(e).position = {2.0f, 2.0f, 0.0f};
    w.emplace<CircleCollider2D>(e).radius = 1.0f;

    Physics2DSystem sys;
    auto hits = sys.overlapCircle(w, {2.5f, 2.0f}, 0.5f);
    REQUIRE(hits.size() == 1);
    REQUIRE(hits[0] == e);

    REQUIRE(sys.overlapBox(w, {10.0f, 10.0f}, {0.5f, 0.5f}).empty());
}
