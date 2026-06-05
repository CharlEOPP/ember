#include "Commands.h"
#include "SceneOps.h"

#include "ember/scene/Scene.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"
#include "ember/renderer/Tilemap.h"
#include "ember/serialization/YAMLSerializer.h"

#include <glm/glm.hpp>

namespace ember::Commands {

std::unique_ptr<Command> create(Scene& scene, CreateKind kind, Entity parent,
                                std::shared_ptr<Entity> created) {
    Scene* s = &scene;
    auto make = [s, kind, parent, created] {
        Entity e = NULL_ENTITY;
        switch (kind) {
            case CreateKind::Empty:  e = SceneOps::createEmpty(*s);  break;
            case CreateKind::Sprite: e = SceneOps::createSprite(*s); break;
            case CreateKind::Camera: e = SceneOps::createCamera(*s); break;
        }
        if (parent != NULL_ENTITY) SceneOps::reparent(*s, e, parent);
        *created = e;
    };
    auto undo = [s, created] { if (*created != NULL_ENTITY) s->destroy(*created); };
    return std::make_unique<FunctionalCommand>("Create Entity", std::move(make), std::move(undo));
}

std::unique_ptr<Command> remove(Scene& scene, Entity e) {
    Scene* s = &scene;
    // Shared state round-trips the deleted sub-tree across undo/redo.
    auto state = std::make_shared<Entity>(e);
    auto yaml  = std::make_shared<std::string>();
    auto pos   = std::make_shared<glm::vec3>(0.0f);
    auto del = [s, state, yaml, pos] {
        if (const Transform* t = s->world().tryGet<Transform>(*state)) *pos = t->position;
        *yaml = YAMLSerializer::serializePrefab(*s, *state);
        s->destroy(*state);
    };
    auto undo = [s, state, yaml, pos] {
        auto res = YAMLDeserializer::instantiatePrefab(*s, *yaml, *pos);
        *state = res ? res.value() : NULL_ENTITY;
    };
    return std::make_unique<FunctionalCommand>("Delete Entity", std::move(del), std::move(undo));
}

std::unique_ptr<Command> duplicate(Scene& scene, Entity src, std::shared_ptr<Entity> created) {
    Scene* s = &scene;
    auto make = [s, src, created] { *created = SceneOps::duplicate(*s, src); };
    auto undo = [s, created] { if (*created != NULL_ENTITY) s->destroy(*created); };
    return std::make_unique<FunctionalCommand>("Duplicate Entity", std::move(make), std::move(undo));
}

std::unique_ptr<Command> reparent(Scene& scene, Entity child, Entity oldParent, Entity newParent) {
    Scene* s = &scene;
    auto doFn   = [s, child, newParent] { SceneOps::reparent(*s, child, newParent); };
    auto undoFn = [s, child, oldParent] { SceneOps::reparent(*s, child, oldParent); };
    return std::make_unique<FunctionalCommand>("Reparent Entity", std::move(doFn), std::move(undoFn));
}

std::unique_ptr<Command> rename(Scene& scene, Entity e, std::string oldName, std::string newName) {
    Scene* s = &scene;
    auto doFn   = [s, e, newName] { SceneOps::rename(*s, e, newName); };
    auto undoFn = [s, e, oldName] { SceneOps::rename(*s, e, oldName); };
    return std::make_unique<FunctionalCommand>("Rename Entity", std::move(doFn), std::move(undoFn));
}

std::unique_ptr<Command> setComponent(Scene& scene, Entity e, const InspectorEntry& entry,
                                      std::shared_ptr<void> before, std::shared_ptr<void> after) {
    Scene* s = &scene;
    auto restore = entry.restore;   // copy the type-erased restore thunk
    auto doFn   = [s, e, restore, after]  { restore(s->world(), e, after);  s->markTransformDirty(e); };
    auto undoFn = [s, e, restore, before] { restore(s->world(), e, before); s->markTransformDirty(e); };
    return std::make_unique<FunctionalCommand>("Edit " + entry.name, std::move(doFn), std::move(undoFn));
}

std::unique_ptr<Command> transformEntities(Scene& scene,
                                           std::vector<EntityXform> before,
                                           std::vector<EntityXform> after) {
    Scene* s = &scene;
    auto apply = [s](const std::vector<EntityXform>& states) {
        for (const EntityXform& x : states)
            if (Transform* t = s->world().tryGet<Transform>(x.e)) {
                *t = x.transform;
                s->markTransformDirty(x.e);
            }
    };
    auto doFn   = [apply, after]  { apply(after); };
    auto undoFn = [apply, before] { apply(before); };
    return std::make_unique<FunctionalCommand>("Transform", std::move(doFn), std::move(undoFn));
}

std::unique_ptr<Command> setComponentBatch(Scene& scene, const InspectorEntry& entry,
                                           std::vector<Entity> entities,
                                           std::vector<std::shared_ptr<void>> before,
                                           std::vector<std::shared_ptr<void>> after) {
    Scene* s = &scene;
    auto restore = entry.restore;
    auto apply = [s, restore, entities](const std::vector<std::shared_ptr<void>>& states) {
        for (std::size_t i = 0; i < entities.size() && i < states.size(); ++i)
            if (s->world().valid(entities[i])) {
                restore(s->world(), entities[i], states[i]);
                s->markTransformDirty(entities[i]);
            }
    };
    auto doFn   = [apply, after]  { apply(after); };
    auto undoFn = [apply, before] { apply(before); };
    return std::make_unique<FunctionalCommand>("Edit " + entry.name + " (batch)",
                                               std::move(doFn), std::move(undoFn));
}

std::unique_ptr<Command> paintTiles(Scene& scene, Entity tilemapEntity, std::vector<TileEdit> edits) {
    Scene* s = &scene;
    Entity te = tilemapEntity;
    auto doFn = [s, te, edits] {
        if (Tilemap* tm = s->world().tryGet<Tilemap>(te))
            for (const TileEdit& ed : edits) tm->setTile(ed.x, ed.y, ed.newId);
    };
    auto undoFn = [s, te, edits] {
        if (Tilemap* tm = s->world().tryGet<Tilemap>(te))
            for (const TileEdit& ed : edits) tm->setTile(ed.x, ed.y, ed.oldId);
    };
    return std::make_unique<FunctionalCommand>("Paint Tiles", std::move(doFn), std::move(undoFn));
}

std::unique_ptr<Command> composite(std::string name, std::vector<std::unique_ptr<Command>> cmds) {
    auto box = std::make_shared<std::vector<std::unique_ptr<Command>>>(std::move(cmds));
    auto doFn = [box] {
        for (auto& c : *box) c->execute();
    };
    auto undoFn = [box] {
        for (auto it = box->rbegin(); it != box->rend(); ++it) (*it)->undo();
    };
    return std::make_unique<FunctionalCommand>(std::move(name), std::move(doFn), std::move(undoFn));
}

} // namespace ember::Commands
