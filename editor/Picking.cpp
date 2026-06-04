#include "Picking.h"

#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"        // WorldTransform
#include "ember/renderer/Components2D.h" // SpriteRenderer
#include "ember/renderer/Camera.h"      // screenToWorld

#include <limits>

namespace ember::Picking {

Entity pick(World& world, const glm::mat4& viewProjection,
            const glm::vec2& viewportSize, const glm::vec2& mousePx) {
    const glm::vec3 wp = screenToWorld(mousePx, viewProjection, viewportSize);
    const glm::vec2 p{wp.x, wp.y};

    Entity best      = NULL_ENTITY;
    int    bestLayer = std::numeric_limits<int>::min();

    for (auto [e, wt, sprite] : world.view<WorldTransform, SpriteRenderer>()) {
        // Axis-aligned world AABB from the WorldTransform (rotation ignored): the
        // unit quad spans [-0.5, 0.5], so half-extent = basis length * 0.5.
        const glm::vec2 center{wt.matrix[3].x, wt.matrix[3].y};
        const glm::vec2 half{ glm::length(glm::vec2(wt.matrix[0])) * 0.5f,
                              glm::length(glm::vec2(wt.matrix[1])) * 0.5f };

        if (p.x >= center.x - half.x && p.x <= center.x + half.x &&
            p.y >= center.y - half.y && p.y <= center.y + half.y) {
            if (sprite.layer >= bestLayer) { bestLayer = sprite.layer; best = e; }
        }
    }
    return best;
}

} // namespace ember::Picking
