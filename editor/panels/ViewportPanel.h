#pragma once
#include "ember/renderer/Camera.h"
#include "ember/renderer/SpriteRenderSystem.h"
#include "ember/renderer/TilemapRenderSystem.h"
#include "ember/renderer/ParticleSystem.h"
#include "ember/scene/TransformSystem.h"
#include "ember/renderer/RHI.h"
#include "../EditorContext.h"

#include <memory>

namespace ember {

class AssetManager;
class Scene;

// Renders the active scene into an off-screen framebuffer and shows it as an
// ImGui::Image. Owns the edit-mode EditorCamera (pan/zoom while hovered) and the
// render systems used to draw the scene.
class ViewportPanel {
public:
    explicit ViewportPanel(AssetManager& assets);

    void onImGuiRender(EditorContext& ctx, f32 dt, bool playing);

    [[nodiscard]] EditorCamera& camera() { return m_camera; }
    [[nodiscard]] bool hovered() const { return m_hovered; }

private:
    void renderScene(Scene& scene, u32 w, u32 h, f32 dt, bool playing);
    void handleCameraInput();

    [[nodiscard]] f32 currentHalfHeight() const;   // derived from the projection
    [[nodiscard]] f32 worldPerPixel() const;

    AssetManager*                 m_assets = nullptr;
    std::shared_ptr<IFramebuffer> m_fb;
    u32 m_fbW = 0, m_fbH = 0;

    EditorCamera        m_camera;
    TransformSystem     m_transforms;
    SpriteRenderSystem  m_sprites;
    TilemapRenderSystem m_tilemap;
    ParticleSystem      m_particles;

    bool m_hovered = false;
};

} // namespace ember
