#pragma once
#include "ember/renderer/Camera.h"
#include "ember/renderer/SpriteRenderSystem.h"
#include "ember/renderer/TilemapRenderSystem.h"
#include "ember/renderer/ParticleSystem.h"
#include "ember/scene/TransformSystem.h"
#include "ember/renderer/RHI.h"
#include "../EditorContext.h"
#include "../Commands.h"   // Commands::EntityXform

#include <memory>
#include <vector>

namespace ember {

class AssetManager;
class Scene;

// Renders the active scene into an off-screen framebuffer and shows it as an
// ImGui::Image. Owns the edit-mode EditorCamera (pan/zoom while hovered) and the
// render systems used to draw the scene.
class ViewportPanel {
public:
    explicit ViewportPanel(AssetManager& assets);

    enum class GizmoMode  { Translate, Rotate, Scale };
    enum class GizmoSpace { Local, World };

    void onImGuiRender(EditorContext& ctx, f32 dt, bool playing);

    [[nodiscard]] EditorCamera& camera() { return m_camera; }
    [[nodiscard]] bool hovered() const { return m_hovered; }

    void setGizmoMode(GizmoMode m) { m_gizmoMode = m; }
    void setSnap(f32 grid, f32 angle) { m_snapGrid = grid; m_snapAngle = angle; }
    void setCameraSpeeds(f32 pan, f32 zoom) { m_panSpeed = pan; m_zoomSpeed = zoom; }

private:
    void renderScene(Scene& scene, u32 w, u32 h, f32 dt, bool playing, const DebugOverlays& overlays);
    void drawDebugOverlays(Scene& scene, const DebugOverlays& overlays);  // GL lines (Debug builds)
    void drawEntityNames(EditorContext& ctx);                            // ImGui text labels
    void handleCameraInput();
    bool drawGizmo(EditorContext& ctx);   // returns true if it consumed input (no-op w/o ImGuizmo)
    bool paintTilemap(EditorContext& ctx);// returns true if it consumed input

    [[nodiscard]] f32 currentHalfHeight() const;   // derived from the projection
    [[nodiscard]] f32 worldPerPixel() const;

    std::shared_ptr<IFramebuffer> m_fb;
    u32 m_fbW = 0, m_fbH = 0;

    EditorCamera        m_camera;
    TransformSystem     m_transforms;
    SpriteRenderSystem  m_sprites;
    TilemapRenderSystem m_tilemap;
    ParticleSystem      m_particles;

    bool m_hovered = false;

    // Gizmo state.
    GizmoMode  m_gizmoMode  = GizmoMode::Translate;
    GizmoSpace m_gizmoSpace = GizmoSpace::World;
    bool       m_gizmoUsing = false;                   // mid-drag
    std::vector<Commands::EntityXform> m_gizmoBefore;  // captured at drag start
    f32        m_snapGrid  = 1.0f;                      // Ctrl-snap (from EditorSettings)
    f32        m_snapAngle = 15.0f;
    f32        m_panSpeed  = 1.0f;
    f32        m_zoomSpeed = 1.0f;

    // Tilemap paint stroke state.
    bool                          m_painting = false;
    std::vector<Commands::TileEdit> m_stroke;
};

} // namespace ember
