#pragma once
#include "ember/ecs/SystemScheduler.h"
#include <glm/glm.hpp>

namespace ember {

class AssetManager;

// Runtime UI/HUD pass. Each frame it (1) updates UIButton hover/press/click flags
// from the pointer, then (2) draws UIImage / UIButton / UIText entities in a
// self-contained screen-space scene (top-left origin, +y down), ordered by each
// element's `order`. It manages its own Renderer2D::beginScene/endScene, so it
// runs *after* the world render pass, not inside it.
//
// Call setScreenSize() whenever the framebuffer resizes (defaults to 1280x720).
class UISystem : public ISystem {
public:
    explicit UISystem(AssetManager& assets) : m_assets(&assets) {}

    void setScreenSize(const glm::vec2& size) { m_screen = size; }

    void update(World& world, f32 dt) override;

private:
    AssetManager* m_assets = nullptr;
    glm::vec2     m_screen{1280.0f, 720.0f};
};

} // namespace ember
