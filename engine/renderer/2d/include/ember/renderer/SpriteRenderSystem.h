#pragma once
#include "ember/ecs/SystemScheduler.h"

namespace ember {

class AssetManager;

// Render-phase system: submits every (WorldTransform + SpriteRenderer) entity to
// Renderer2D, sorted by layer. Sprite textures are resolved through the
// AssetManager by their AssetHandle; unresolved/empty handles fall back to the
// magenta missing-texture placeholder. The caller wraps the render phase in
// Renderer2D::beginScene/endScene.
class SpriteRenderSystem : public ISystem {
public:
    explicit SpriteRenderSystem(AssetManager& assets) : m_assets(&assets) {}

    void update(World& world, f32 dt) override;

private:
    AssetManager* m_assets = nullptr;
};

} // namespace ember
