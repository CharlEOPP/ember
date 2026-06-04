#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/Reflect.h"
#include "ember/assets/AssetHandle.h"
#include "ember/assets/Texture2D.h"

#include <glm/glm.hpp>
#include <string>

namespace ember {

// A drawable 2D sprite. `texture` is an AssetHandle resolved through the
// AssetManager by SpriteRenderSystem (null handle ⇒ flat color / placeholder).
// `texturePath` is the serializable source; SpriteRenderSystem loads it into
// `texture` when set (Epic 05 migration, MIG-01).
struct SpriteRenderer {
    AssetHandle<Texture2D> texture;
    std::string texturePath;
    glm::vec4   color{1.0f};
    // Sub-rectangle of the texture to display: (minU, minV, maxU, maxV).
    // Defaults to the full texture; SpriteAnimationSystem / tilemaps set it to a
    // single atlas frame (Epic 12, SAN-01).
    glm::vec4   uvRect{0.0f, 0.0f, 1.0f, 1.0f};
    i32         layer  = 0;
    bool        flipX  = false;
    bool        flipY  = false;
};

// Orthographic 2D camera. `size` is the half-height of the view in world units.
struct Camera2D {
    f32  size     = 5.0f;
    f32  nearClip = -1.0f;
    f32  farClip  =  1.0f;
    bool isPrimary = true;
};

// Field reflection (serializable fields only; the texture handle is rebuilt from texturePath).
EMBER_REFLECT(SpriteRenderer) {
    EMBER_FIELD(texturePath);
    EMBER_FIELD(color);
    EMBER_FIELD(uvRect);
    EMBER_FIELD(layer);
    EMBER_FIELD(flipX);
    EMBER_FIELD(flipY);
}

EMBER_REFLECT(Camera2D) {
    EMBER_FIELD(size);
    EMBER_FIELD(nearClip);
    EMBER_FIELD(farClip);
    EMBER_FIELD(isPrimary);
}

} // namespace ember
