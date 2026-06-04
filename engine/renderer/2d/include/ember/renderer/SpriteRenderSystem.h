#pragma once
#include "ember/ecs/SystemScheduler.h"
namespace ember {
// Render-phase system: submits every (WorldTransform + SpriteRenderer) entity to
// Renderer2D, sorted by layer. The caller wraps the render phase in
// Renderer2D::beginScene/endScene.
class SpriteRenderSystem : public ISystem {
public:
    void update(World& world, f32 dt) override;
};
} // namespace ember
