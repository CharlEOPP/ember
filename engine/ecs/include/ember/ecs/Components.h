#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/Reflect.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace ember {

// ---- Built-in components (plain data) ----
struct Transform {
    glm::vec3 position{0.0f};
    f32       rotation = 0.0f;   // radians, 2D (rotation about Z)
    glm::vec3 scale{1.0f};
};

// Cached world-space matrix; rebuilt by TransformSystem. NOT serialized.
struct WorldTransform {
    glm::mat4 matrix{1.0f};
    bool      dirty = true;
};

struct Tag {
    std::string name;
    std::string layer;
};

struct Parent {
    Entity parent = NULL_ENTITY;
};

struct Children {
    std::vector<Entity> children;
};

// ---- Field reflection (header-visible so the serializer can instantiate it) ----
EMBER_REFLECT(Transform) {
    EMBER_FIELD(position);
    EMBER_FIELD(rotation);
    EMBER_FIELD(scale);
}

EMBER_REFLECT(Tag) {
    EMBER_FIELD(name);
    EMBER_FIELD(layer);
}

EMBER_REFLECT(Parent) {
    EMBER_FIELD(parent);
}

EMBER_REFLECT(Children) {
    EMBER_FIELD(children);
}

} // namespace ember
