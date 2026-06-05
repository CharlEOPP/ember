#include <catch2/catch_test_macros.hpp>
#include "GizmoMath.h"
#include "Commands.h"
#include "CommandHistory.h"

#include "ember/scene/Scene.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"

#include <vector>

using namespace ember;

TEST_CASE("GizmoMath snaps translate + angle", "[editor][gizmo]") {
    const glm::vec3 s = GizmoMath::snapTranslate({1.2f, 2.7f, -0.4f}, 1.0f);
    REQUIRE(s.x == 1.0f);
    REQUIRE(s.y == 3.0f);
    REQUIRE(s.z == 0.0f);
    REQUIRE(GizmoMath::snapTranslate({1.2f, 0, 0}, 0.0f).x == 1.2f);   // grid<=0 -> unchanged
    REQUIRE(GizmoMath::snapAngleDeg(20.0f, 15.0f) == 15.0f);
    REQUIRE(GizmoMath::snapAngleDeg(23.0f, 15.0f) == 30.0f);
}

TEST_CASE("transformEntities applies + undoes for all entities", "[editor][gizmo]") {
    Scene scene("t");
    Entity a = scene.create("A");
    Entity b = scene.create("B");
    scene.world().get<Transform>(a).position = {0, 0, 0};
    scene.world().get<Transform>(b).position = {5, 0, 0};

    std::vector<Commands::EntityXform> before, after;
    before.push_back({a, scene.world().get<Transform>(a)});
    before.push_back({b, scene.world().get<Transform>(b)});
    Transform na = before[0].transform; na.position += glm::vec3(1, 2, 0);
    Transform nb = before[1].transform; nb.position += glm::vec3(1, 2, 0);
    after.push_back({a, na});
    after.push_back({b, nb});

    CommandHistory h;
    h.push(Commands::transformEntities(scene, before, after));
    REQUIRE(scene.world().get<Transform>(a).position == glm::vec3(1, 2, 0));
    REQUIRE(scene.world().get<Transform>(b).position == glm::vec3(6, 2, 0));

    h.undo();
    REQUIRE(scene.world().get<Transform>(a).position == glm::vec3(0, 0, 0));
    REQUIRE(scene.world().get<Transform>(b).position == glm::vec3(5, 0, 0));
}
