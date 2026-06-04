#pragma once
#include "ember/ecs/Entity.h"
#include <glm/glm.hpp>

namespace ember {

class World;

// CPU entity picking (ImGui-free, headless-testable). Unprojects a viewport-local
// pixel (top-left origin) through the camera and returns the front-most sprite
// entity whose world-space AABB contains the point (highest SpriteRenderer.layer
// wins; ties resolve to the later-iterated entity). NULL_ENTITY if none hit.
namespace Picking {

Entity pick(World& world, const glm::mat4& viewProjection,
            const glm::vec2& viewportSize, const glm::vec2& mousePx);

} // namespace Picking
} // namespace ember
