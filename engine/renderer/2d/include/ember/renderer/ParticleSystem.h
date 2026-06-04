#pragma once
#include "ember/ecs/SystemScheduler.h"   // ISystem, World

namespace ember {

// Updates and renders every (Transform + ParticleEmitter) entity: advances the
// emitter simulation, then draws each live particle as a batched quad through
// Renderer2D (size/color lerped over its life). Call within beginScene/endScene.
class ParticleSystem : public ISystem {
public:
    void update(World& world, f32 dt) override;
};

} // namespace ember
