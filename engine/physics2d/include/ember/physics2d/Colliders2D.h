#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Reflect.h"
#include <glm/glm.hpp>

namespace ember {

// Surface response properties. friction 0..1, restitution 0 (inelastic) .. 1 (bouncy).
struct PhysicsMaterial2D {
    f32 friction    = 0.2f;
    f32 restitution = 0.0f;
};

// Axis-aligned box collider. `halfExtents`/`offset` are local; world placement
// scales by the entity's Transform.scale (x,y).
struct BoxCollider2D {
    glm::vec2         halfExtents{0.5f, 0.5f};
    glm::vec2         offset{0.0f};
    PhysicsMaterial2D material{};
    bool              isTrigger = false;
};

struct CircleCollider2D {
    f32               radius = 0.5f;
    glm::vec2         offset{0.0f};
    PhysicsMaterial2D material{};
    bool              isTrigger = false;
};

EMBER_REFLECT(BoxCollider2D) {
    EMBER_FIELD(halfExtents);
    EMBER_FIELD(offset);
    EMBER_FIELD(isTrigger);
}

EMBER_REFLECT(CircleCollider2D) {
    EMBER_FIELD(radius);
    EMBER_FIELD(offset);
    EMBER_FIELD(isTrigger);
}

} // namespace ember
