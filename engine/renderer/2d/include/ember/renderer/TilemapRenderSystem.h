#pragma once
#include "ember/ecs/SystemScheduler.h"   // ISystem, World

namespace ember {

class AssetManager;

// Renders every (Transform + Tilemap) entity by batching its non-empty tiles
// through Renderer2D. Resolves the tileset texture via the AssetManager. Wrap
// the render phase in Renderer2D::beginScene/endScene as usual.
class TilemapRenderSystem : public ISystem {
public:
    explicit TilemapRenderSystem(AssetManager& assets) : m_assets(&assets) {}
    void update(World& world, f32 dt) override;

private:
    AssetManager* m_assets = nullptr;
};

} // namespace ember
