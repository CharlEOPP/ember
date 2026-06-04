#pragma once
#include "ember/ecs/SystemScheduler.h"   // ISystem, World

namespace ember {

class AssetManager;

// Advances every SpriteAnimator and writes the active frame's uvRect into the
// entity's SpriteRenderer. Run at Phase::Update, before the sprite render system.
class SpriteAnimationSystem : public ISystem {
public:
    explicit SpriteAnimationSystem(AssetManager& assets) : m_assets(&assets) {}
    void update(World& world, f32 dt) override;

private:
    AssetManager* m_assets = nullptr;
};

} // namespace ember
