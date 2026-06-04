#include <catch2/catch_test_macros.hpp>
#include "Picking.h"

#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"
#include "ember/renderer/Components2D.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace ember;

namespace {
glm::mat4 orthoVP() {   // half-height 10, square; centered at origin
    return glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -1.0f, 1.0f);
}
Entity placeSprite(World& w, glm::vec2 pos, glm::vec2 scale, int layer) {
    Entity e = w.create();
    auto& wt = w.emplace<WorldTransform>(e);
    wt.matrix = glm::translate(glm::mat4(1.0f), glm::vec3(pos, 0.0f))
              * glm::scale(glm::mat4(1.0f), glm::vec3(scale, 1.0f));
    auto& s = w.emplace<SpriteRenderer>(e);
    s.layer = layer;
    return e;
}
} // namespace

TEST_CASE("Picking maps a click to the entity under it", "[editor][picking]") {
    World w;
    const glm::vec2 vp{200.0f, 200.0f};
    const glm::mat4 cam = orthoVP();

    Entity a = placeSprite(w, {0.0f, 0.0f}, {2.0f, 2.0f}, 0);   // covers [-1,1]
    Entity b = placeSprite(w, {5.0f, 0.0f}, {2.0f, 2.0f}, 1);   // covers x in [4,6]

    REQUIRE(Picking::pick(w, cam, vp, {100.0f, 100.0f}) == a);  // screen center -> world (0,0)
    REQUIRE(Picking::pick(w, cam, vp, {150.0f, 100.0f}) == b);  // world (5,0)
    REQUIRE(Picking::pick(w, cam, vp, {190.0f, 10.0f})  == NULL_ENTITY);  // empty
}

TEST_CASE("Picking returns the front-most (highest layer) on overlap", "[editor][picking]") {
    World w;
    const glm::vec2 vp{200.0f, 200.0f};
    const glm::mat4 cam = orthoVP();

    placeSprite(w, {0.0f, 0.0f}, {2.0f, 2.0f}, 0);
    Entity top = placeSprite(w, {0.0f, 0.0f}, {2.0f, 2.0f}, 5);
    REQUIRE(Picking::pick(w, cam, vp, {100.0f, 100.0f}) == top);
}

TEST_CASE("Picking is correct at a non-square viewport", "[editor][picking]") {
    World w;
    const glm::vec2 vp{400.0f, 200.0f};
    const glm::mat4 cam = orthoVP();
    Entity a = placeSprite(w, {0.0f, 0.0f}, {2.0f, 2.0f}, 0);
    // Screen center still maps to world origin regardless of viewport aspect.
    REQUIRE(Picking::pick(w, cam, vp, {200.0f, 100.0f}) == a);
}
