#include "ViewportPanel.h"
#include "../Picking.h"

#include "ember/renderer/Renderer2D.h"
#include "ember/scene/Scene.h"
#include "ember/ecs/World.h"

#include <imgui.h>
#include <algorithm>
#include <cstdio>

namespace ember {

ViewportPanel::ViewportPanel(AssetManager& assets)
    : m_sprites(assets), m_tilemap(assets) {}

f32 ViewportPanel::currentHalfHeight() const {
    const f32 yScale = m_camera.viewProjection()[1][1];   // = 1 / halfHeight
    return (yScale != 0.0f) ? 1.0f / yScale : 5.0f;
}

f32 ViewportPanel::worldPerPixel() const {
    return (m_fbH != 0) ? (2.0f * currentHalfHeight()) / static_cast<f32>(m_fbH) : 0.0f;
}

void ViewportPanel::renderScene(Scene& scene, u32 w, u32 h, f32 dt, bool playing) {
    if (!m_fb) {
        FramebufferSpec spec; spec.width = w; spec.height = h;
        m_fb = RHI::createFramebuffer(spec);
        m_fbW = w; m_fbH = h;
        m_camera.setViewportSize(w, h);
    } else if (w != m_fbW || h != m_fbH) {
        m_fb->resize(w, h);
        m_fbW = w; m_fbH = h;
        m_camera.setViewportSize(w, h);
    }

    m_fb->bind();
    RHI::setViewport(0, 0, w, h);
    const glm::vec4 bg = scene.settings().backgroundColor;
    RHI::setClearColor(bg.r, bg.g, bg.b, bg.a);
    RHI::clear();

    World& world = scene.world();
    // While playing, physics moves Transforms without setting the dirty flag, so
    // refresh every WorldTransform before the sprite pass.
    if (playing)
        for (auto [e, wt] : world.view<WorldTransform>()) wt.dirty = true;
    m_transforms.update(world, 0.0f);

    const f32 simDt = playing ? dt : 0.0f;   // particles simulate only in Play mode
    Renderer2D::beginScene(m_camera.viewProjection());
    m_tilemap.update(world, 0.0f);
    m_sprites.update(world, 0.0f);
    m_particles.update(world, simDt);
    Renderer2D::endScene();

    m_fb->unbind();
}

void ViewportPanel::handleCameraInput() {
    if (!m_hovered) return;
    ImGuiIO& io = ImGui::GetIO();

    if (io.MouseWheel != 0.0f)
        m_camera.zoom(io.MouseWheel * 0.1f * currentHalfHeight());   // proportional zoom

    // Pan with middle-mouse (or Alt+left) drag.
    const bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
                          (io.KeyAlt && ImGui::IsMouseDragging(ImGuiMouseButton_Left));
    if (dragging) {
        const f32 wpp = worldPerPixel();
        m_camera.pan(glm::vec2(-io.MouseDelta.x * wpp, io.MouseDelta.y * wpp));
    }
}

void ViewportPanel::onImGuiRender(EditorContext& ctx, f32 dt, bool playing) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const u32 w = static_cast<u32>(std::max(1.0f, avail.x));
    const u32 h = static_cast<u32>(std::max(1.0f, avail.y));

    if (ctx.scene) renderScene(*ctx.scene, w, h, dt, playing);

    if (m_fb) {
        ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(m_fb->getColorAttachment())),
                     ImVec2(static_cast<float>(w), static_cast<float>(h)),
                     ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));   // flip V (GL origin)
    }
    m_hovered = ImGui::IsItemHovered();

    // Left-click (no drag) picks the entity under the cursor.
    if (!playing && ctx.scene && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mn = ImGui::GetItemRectMin();
        const ImVec2 mp = ImGui::GetMousePos();
        const glm::vec2 local{mp.x - mn.x, mp.y - mn.y};
        ctx.selected = Picking::pick(ctx.scene->world(), m_camera.viewProjection(),
                                     glm::vec2(static_cast<f32>(m_fbW), static_cast<f32>(m_fbH)), local);
    }

    if (!playing) handleCameraInput();   // game owns input while playing

    // Overlay: renderer stats (top-left of the image).
    const ImVec2 imgMin = ImGui::GetItemRectMin();
    const RendererStats& s = Renderer2D::stats();
    char buf[96];
    std::snprintf(buf, sizeof(buf), "draws %u  quads %u", s.drawCalls, s.quadCount);
    ImGui::GetWindowDrawList()->AddText(ImVec2(imgMin.x + 8.0f, imgMin.y + 6.0f),
                                        IM_COL32(220, 220, 220, 200), buf);

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace ember
