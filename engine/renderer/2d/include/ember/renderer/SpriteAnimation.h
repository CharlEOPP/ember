#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Reflect.h"
#include "ember/assets/AssetHandle.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace ember {

// An ordered set of sprite-sheet frames played at a fixed rate. Each frame is a
// UV sub-rect (minU, minV, maxU, maxV) into the sprite's texture. Loaded as an
// asset (see SpriteAnimationClipLoader).
struct SpriteAnimationClip {
    std::vector<glm::vec4> frames;   // each = uvRect
    f32  fps  = 12.0f;
    bool loop = true;

    [[nodiscard]] f32 duration() const {
        return (fps > 0.0f && !frames.empty()) ? static_cast<f32>(frames.size()) / fps : 0.0f;
    }
};

// Plays a SpriteAnimationClip, writing the active frame's uvRect into the
// entity's SpriteRenderer each frame (SpriteAnimationSystem).
struct SpriteAnimator {
    AssetHandle<SpriteAnimationClip> clip;
    std::string clipPath;            // serialized source; lazily loaded into `clip`
    f32  time    = 0.0f;             // runtime playback time (not serialized)
    f32  speed   = 1.0f;
    bool playing = true;
};

EMBER_REFLECT(SpriteAnimator) {
    EMBER_FIELD(clipPath);
    EMBER_FIELD(speed);
    EMBER_FIELD(playing);
}

} // namespace ember
