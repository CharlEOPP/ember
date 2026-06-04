#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/Reflect.h"
#include "ember/renderer/RHI.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace ember {

// A drawable 2D sprite. `texture` may be null (flat color). `texturePath` is the
// serializable source; the shared_ptr is rebuilt from it on load (Epic 05 will
// migrate this to AssetHandle<Texture2D>).
struct SpriteRenderer {
    std::shared_ptr<ITexture2D> texture;
    std::string texturePath;
    glm::vec4   color{1.0f};
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
