#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ember/renderer/Camera.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"

#include <glm/glm.hpp>

using namespace ember;
using Catch::Approx;

TEST_CASE("Camera_OrthoProjectionAspect", "[renderer][camera]") {
    World world;
    Entity cam = world.create();
    world.emplace<Camera2D>(cam);              // size 5, isPrimary
    world.emplace<WorldTransform>(cam);        // identity at origin

    CameraSystem system;
    system.setViewportSize(800, 600);          // aspect 4:3
    system.update(world, 0.0f);
    REQUIRE(system.hasCamera());

    const glm::mat4 vp = system.viewProjection();
    const glm::vec2 viewport{ 800.0f, 600.0f };

    // World origin maps to screen centre.
    glm::vec2 centre = worldToScreen(glm::vec3(0.0f), vp, viewport);
    REQUIRE(centre.x == Approx(400.0f).margin(0.01));
    REQUIRE(centre.y == Approx(300.0f).margin(0.01));

    // Round-trip a non-trivial world point.
    glm::vec3 p{ 2.0f, -1.5f, 0.0f };
    glm::vec2 screen = worldToScreen(p, vp, viewport);
    glm::vec3 back   = screenToWorld(screen, vp, viewport);
    REQUIRE(back.x == Approx(p.x).margin(0.001));
    REQUIRE(back.y == Approx(p.y).margin(0.001));
}
