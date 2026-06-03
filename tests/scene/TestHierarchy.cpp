#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ember/scene/Scene.h"
#include "ember/ecs/SystemScheduler.h"

#include <glm/glm.hpp>

using namespace ember;
using Catch::Approx;

TEST_CASE("Scene_FindByNameAndTag", "[scene]") {
    Scene s("test");
    Entity a = s.create("Alpha");
    Entity b = s.create("Beta");
    s.world().get<Tag>(b).layer = "enemies";

    REQUIRE(s.findByName("Alpha") == a);
    REQUIRE(s.findByName("Nope")  == NULL_ENTITY);

    auto enemies = s.findByTag("enemies");
    REQUIRE(enemies.size() == 1u);
    REQUIRE(enemies[0] == b);
}

TEST_CASE("Scene_SetGetParent", "[scene]") {
    Scene s("t");
    Entity p  = s.create("P");
    Entity c  = s.create("C");
    Entity p2 = s.create("P2");

    s.setParent(c, p);
    REQUIRE(s.getParent(c) == p);
    REQUIRE(s.world().get<Children>(p).children.size() == 1u);

    s.setParent(c, p2);   // reparent removes from old parent
    REQUIRE(s.getParent(c) == p2);
    REQUIRE(s.world().get<Children>(p).children.empty());
    REQUIRE(s.world().get<Children>(p2).children.size() == 1u);
}

TEST_CASE("Transform_WorldPropagation", "[scene]") {
    Scene s("t");
    Entity a = s.create("A");
    Entity b = s.create("B");
    Entity c = s.create("C");
    s.world().get<Transform>(a).position = {1.0f, 0.0f, 0.0f};
    s.world().get<Transform>(b).position = {0.0f, 2.0f, 0.0f};
    s.world().get<Transform>(c).position = {0.0f, 0.0f, 3.0f};
    s.setParent(b, a);
    s.setParent(c, b);

    glm::mat4 w = s.getWorldTransform(c);
    glm::vec3 pos = glm::vec3(w[3]);   // translation column
    REQUIRE(pos.x == Approx(1.0f));
    REQUIRE(pos.y == Approx(2.0f));
    REQUIRE(pos.z == Approx(3.0f));
}

TEST_CASE("Transform_DirtyOnMutation", "[scene]") {
    Scene s("t");
    Entity a = s.create("A");
    Entity b = s.create("B");
    s.setParent(b, a);

    s.getWorldTransform(b);                                  // resolves -> clean
    REQUIRE_FALSE(s.world().get<WorldTransform>(b).dirty);

    s.world().get<Transform>(a).position = {5.0f, 0.0f, 0.0f};
    s.markTransformDirty(a);                                 // marks a + descendants
    REQUIRE(s.world().get<WorldTransform>(b).dirty);
}
