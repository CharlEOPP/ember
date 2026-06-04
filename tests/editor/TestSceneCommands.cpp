#include <catch2/catch_test_macros.hpp>
#include "Commands.h"
#include "SceneOps.h"
#include "CommandHistory.h"

#include "ember/scene/Scene.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"

#include <memory>

using namespace ember;

TEST_CASE("Create command adds an entity; undo removes it", "[editor][command]") {
    Scene scene("t");
    CommandHistory h;
    auto created = std::make_shared<Entity>(NULL_ENTITY);

    h.push(Commands::create(scene, Commands::CreateKind::Empty, NULL_ENTITY, created));
    REQUIRE(*created != NULL_ENTITY);
    REQUIRE(scene.world().valid(*created));

    h.undo();
    REQUIRE_FALSE(scene.world().valid(*created));
    h.redo();
    REQUIRE(scene.world().valid(*created));
}

TEST_CASE("Rename command round-trips the tag name", "[editor][command]") {
    Scene scene("t");
    Entity e = SceneOps::createEmpty(scene, "Original");
    CommandHistory h;

    h.push(Commands::rename(scene, e, "Original", "Renamed"));
    REQUIRE(scene.world().get<Tag>(e).name == "Renamed");
    h.undo();
    REQUIRE(scene.world().get<Tag>(e).name == "Original");
    h.redo();
    REQUIRE(scene.world().get<Tag>(e).name == "Renamed");
}

TEST_CASE("Reparent command sets/clears the parent; undo restores it", "[editor][command]") {
    Scene scene("t");
    Entity parent = SceneOps::createEmpty(scene, "P");
    Entity child  = SceneOps::createEmpty(scene, "C");
    CommandHistory h;

    h.push(Commands::reparent(scene, child, /*old*/ NULL_ENTITY, /*new*/ parent));
    REQUIRE(scene.getParent(child) == parent);
    h.undo();
    REQUIRE(scene.getParent(child) == NULL_ENTITY);
}

TEST_CASE("SceneOps reparent rejects cycles and self-parenting", "[editor][sceneops]") {
    Scene scene("t");
    Entity a = SceneOps::createEmpty(scene, "A");
    Entity b = SceneOps::createEmpty(scene, "B");
    REQUIRE(SceneOps::reparent(scene, b, a));         // b under a
    REQUIRE_FALSE(SceneOps::reparent(scene, a, b));   // a under its own descendant
    REQUIRE_FALSE(SceneOps::reparent(scene, a, a));   // self
}
