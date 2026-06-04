#include "ember/renderer/SpriteAnimationSystem.h"
#include "ember/renderer/SpriteAnimation.h"
#include "ember/renderer/Components2D.h"
#include "ember/assets/AssetManager.h"
#include "ember/ecs/World.h"

#include <cmath>

namespace ember {

void SpriteAnimationSystem::update(World& world, f32 dt) {
    for (auto [e, anim, sprite] : world.view<SpriteAnimator, SpriteRenderer>()) {
        // Lazily resolve the clip from its path on first use.
        if (!anim.clip.valid() && !anim.clipPath.empty() && m_assets)
            anim.clip = m_assets->load<SpriteAnimationClip>(anim.clipPath);

        SpriteAnimationClip* clip = m_assets ? m_assets->get<SpriteAnimationClip>(anim.clip) : nullptr;
        if (!clip || clip->frames.empty() || clip->fps <= 0.0f) continue;

        if (anim.playing) anim.time += dt * anim.speed;

        const f32   dur = clip->duration();
        const usize n   = clip->frames.size();
        usize frame;
        if (clip->loop) {
            f32 t = std::fmod(anim.time, dur);
            if (t < 0.0f) t += dur;
            frame = static_cast<usize>(t * clip->fps);
            if (frame >= n) frame = 0;                 // guard fp edge at the wrap
        } else {
            frame = static_cast<usize>(anim.time * clip->fps);
            if (frame >= n) { frame = n - 1; anim.playing = false; }   // clamp + stop
        }
        sprite.uvRect = clip->frames[frame];
    }
}

} // namespace ember
