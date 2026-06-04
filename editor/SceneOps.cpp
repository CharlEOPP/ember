#include "SceneOps.h"

#include "ember/scene/Scene.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"
#include "ember/renderer/Components2D.h"          // SpriteRenderer, Camera2D
#include "ember/serialization/YAMLSerializer.h"

namespace ember::SceneOps {

Entity createEmpty(Scene& scene, const std::string& name) {
    return scene.create(name);
}

Entity createSprite(Scene& scene, const std::string& name) {
    Entity e = scene.create(name);
    scene.world().emplace<SpriteRenderer>(e);
    return e;
}

Entity createCamera(Scene& scene, const std::string& name) {
    Entity e = scene.create(name);
    scene.world().emplace<Camera2D>(e);
    return e;
}

Entity duplicate(Scene& scene, Entity e) {
    if (!scene.world().valid(e)) return NULL_ENTITY;
    glm::vec3 pos{0.0f};
    if (const Transform* t = scene.world().tryGet<Transform>(e)) pos = t->position;
    const std::string yaml = YAMLSerializer::serializePrefab(scene, e);
    auto res = YAMLDeserializer::instantiatePrefab(scene, yaml, pos);
    return res ? res.value() : NULL_ENTITY;
}

void rename(Scene& scene, Entity e, const std::string& name) {
    if (Tag* tag = scene.world().tryGet<Tag>(e)) tag->name = name;
}

bool isAncestorOrSelf(Scene& scene, Entity ancestor, Entity e) {
    Entity cur = e;
    while (cur != NULL_ENTITY) {
        if (cur == ancestor) return true;
        cur = scene.getParent(cur);
    }
    return false;
}

bool reparent(Scene& scene, Entity child, Entity parent) {
    if (child == NULL_ENTITY || child == parent) return false;
    // Rejecting when `child` is an ancestor of (or equals) `parent` prevents cycles.
    if (parent != NULL_ENTITY && isAncestorOrSelf(scene, child, parent)) return false;
    scene.setParent(child, parent);
    return true;
}

std::vector<Entity> roots(Scene& scene) {
    std::vector<Entity> out;
    for (auto [e, tag] : scene.world().view<Tag>()) {
        (void)tag;
        const Parent* p = scene.world().tryGet<Parent>(e);
        if (!p || p->parent == NULL_ENTITY) out.push_back(e);
    }
    return out;
}

std::vector<Entity> childrenOf(Scene& scene, Entity e) {
    if (const Children* ch = scene.world().tryGet<Children>(e)) return ch->children;
    return {};
}

} // namespace ember::SceneOps
