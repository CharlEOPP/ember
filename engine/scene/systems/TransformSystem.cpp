#include "ember/scene/TransformSystem.h"
#include "ember/ecs/World.h"
#include <glm/gtc/matrix_transform.hpp>
namespace ember {
glm::mat4 localMatrix(const Transform& t) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), t.position);
    m = glm::rotate(m, t.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    m = glm::scale(m, t.scale);
    return m;
}
static Entity parentOf(World& world, Entity e) {
    const Parent* p = world.tryGet<Parent>(e);
    return p ? p->parent : NULL_ENTITY;
}
glm::mat4 resolveWorldTransform(World& world, Entity e) {
    WorldTransform* wt = world.tryGet<WorldTransform>(e);
    if (wt && !wt->dirty) return wt->matrix;
    const Transform* tr = world.tryGet<Transform>(e);
    const glm::mat4 local = tr ? localMatrix(*tr) : glm::mat4(1.0f);
    const Entity p = parentOf(world, e);
    const glm::mat4 worldMat = (p == NULL_ENTITY) ? local : resolveWorldTransform(world, p) * local;
    if (wt) { wt->matrix = worldMat; wt->dirty = false; }
    return worldMat;
}
void TransformSystem::update(World& world, f32 /*dt*/) {
    for (auto [e, tr, wt] : world.view<Transform, WorldTransform>()) {
        (void)tr;
        if (wt.dirty) resolveWorldTransform(world, e);
    }
}
} // namespace ember
