#pragma once
#include "ember/ecs/SystemScheduler.h"
#include "ember/ecs/Components.h"

#include <glm/glm.hpp>

namespace ember {

// Local TRS matrix from a Transform (2D: rotation about Z).
glm::mat4 localMatrix(const Transform& t);

// Recompute (and cache) an entity's world-space matrix, resolving its parent
// chain first. Respects the WorldTransform dirty flag.
glm::mat4 resolveWorldTransform(World& world, Entity e);

// Runs in Phase::PreUpdate: rebuilds every dirty WorldTransform, parent-first.
struct TransformSystem : ISystem {
    void update(World& world, f32 dt) override;
};

} // namespace ember
