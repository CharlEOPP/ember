#pragma once
#include "ember/ecs/SystemScheduler.h"
#include "ember/ecs/Components.h"
#include <glm/glm.hpp>
namespace ember {
glm::mat4 localMatrix(const Transform& t);
glm::mat4 resolveWorldTransform(World& world, Entity e);
struct TransformSystem : ISystem {
    void update(World& world, f32 dt) override;
};
} // namespace ember
