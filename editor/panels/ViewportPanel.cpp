#include "ViewportPanel.h"
#include "../Picking.h"
#include "../GizmoMath.h"
#include "../CommandHistory.h"
#include "../TilemapPaint.h"
#include "ember/renderer/Tilemap.h"

#include "ember/renderer/Renderer2D.h"
#include "ember/renderer/DebugDraw.h"
#include "ember/renderer/Components2D.h"
#include "ember/physics2d/Colliders2D.h"
#include "ember/scene/Scene.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"
#include "ember/core/Profiler.h"

#include <imgui.h>
#ifdef EMBER_HAS_IMGUIZMO
#include <ImGuizmo.h>
#endif
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstdio>
#include <string>

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

void ViewportPanel::renderScene(Scene& scene, u32 w, u32 h, f32 dt, bool playing,
                                const DebugOverlays& overlays) {
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
    {
        EMBER_PROFILE_SCOPE("Render2D");
        Renderer2D::beginScene(m_camera.viewProjection());
        m_tilemap.update(world, 0.0f);
        m_sprites.update(world, 0.0f);
        m_particles.update(world, simDt);
        Renderer2D::endScene();
    }

    drawDebugOverlays(scene, overlays);   // GL lines into the same FBO (Debug builds)

    m_fb->unbind();
}

void ViewportPanel::drawDebugOverlays(Scene& scene, const DebugOverlays& ov) {
#if !defined(NDEBUG)
    if (!ov.grid && !ov.colliders && !ov.frustums) return;
    World& world = scene.world();
    DebugDraw::begin(m_camera.viewProjection());

    if (ov.grid && ov.gridCell > 0.0f) {
        const float ext = 50.0f;
        const glm::vec4 c{0.3f, 0.3f, 0.3f, 1.0f};
        for (float g = -ext; g <= ext; g += ov.gridCell) {
            DebugDraw::line({g, -ext}, {g, ext}, c);
            DebugDraw::line({-ext, g}, {ext, g}, c);
        }
    }
    if (ov.colliders) {
        const glm::vec4 c{0.2f, 0.9f, 0.3f, 1.0f};
        for (auto [e, t, bc] : world.view<Transform, BoxCollider2D>()) {
            const glm::vec2 center = glm::vec2(t.position) + bc.offset;
            const glm::vec2 size{bc.halfExtents.x * 2.0f * t.scale.x, bc.halfExtents.y * 2.0f * t.scale.y};
            DebugDraw::rect(center, size, c);
        }
        for (auto [e, t, cc] : world.view<Transform, CircleCollider2D>())
            DebugDraw::circle(glm::vec2(t.position) + cc.offset, cc.radius * t.scale.x, c);
    }
    if (ov.frustums) {
        const float aspect = (m_fbH != 0) ? static_cast<float>(m_fbW) / static_cast<float>(m_fbH) : 1.0f;
        const glm::vec4 c{0.95f, 0.85f, 0.2f, 1.0f};
        for (auto [e, t, cam] : world.view<Transform, Camera2D>())
            DebugDraw::rect(glm::vec2(t.position), {cam.size * 2.0f * aspect, cam.size * 2.0f}, c);
    }

    DebugDraw::flush();
#else
    (void)scene; (void)ov;
#endif
}

void ViewportPanel::drawEntityNames(EditorContext& ctx) {
    if (!ctx.overlays.names || !ctx.scene) return;
    const ImVec2 mn = ImGui::GetItemRectMin();
    const glm::vec2 vp(static_cast<f32>(m_fbW), static_cast<f32>(m_fbH));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (auto [e, wt, tag] : ctx.scene->world().view<WorldTransform, Tag>()) {
        const glm::vec3 world{wt.matrix[3].x, wt.matrix[3].y, 0.0f};
        const glm::vec2 sp = worldToScreen(world, m_camera.viewProjection(), vp);
        if (sp.x < 0 || sp.y < 0 || sp.x > vp.x || sp.y > vp.y) continue;
        dl->AddText(ImVec2(mn.x + sp.x, mn.y + sp.y), IM_COL32(255, 255, 255, 220), tag.name.c_str());
    }
}

void ViewportPanel::handleCameraInput() {
    if (!m_hovered) return;
    ImGuiIO& io = ImGui::GetIO();

    if (io.MouseWheel != 0.0f)
        m_camera.zoom(io.MouseWheel * 0.1f * m_zoomSpeed * currentHalfHeight());   // proportional zoom

    // Pan with middle-mouse (or Alt+left) drag.
    const bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
                          (io.KeyAlt && ImGui::IsMouseDragging(ImGuiMouseButton_Left));
    if (dragging) {
        const f32 wpp = worldPerPixel() * m_panSpeed;
        m_camera.pan(glm::vec2(-io.MouseDelta.x * wpp, io.MouseDelta.y * wpp));
    }
}

void ViewportPanel::onImGuiRender(EditorContext& ctx, f32 dt, bool playing) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const u32 w = static_cast<u32>(std::max(1.0f, avail.x));
    const u32 h = static_cast<u32>(std::max(1.0f, avail.y));

    if (ctx.scene) renderScene(*ctx.scene, w, h, dt, playing, ctx.overlays);

    if (m_fb) {
        ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(m_fb->getColorAttachment())),
                     ImVec2(static_cast<float>(w), static_cast<float>(h)),
                     ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));   // flip V (GL origin)
    }
    m_hovered = ImGui::IsItemHovered();

    // Drop a .eprefab from the Asset Browser into the viewport to instantiate it.
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("EMBER_ASSET_PATH")) {
            const std::string path(static_cast<const char*>(p->Data));
            if (path.size() >= 8 && path.substr(path.size() - 8) == ".eprefab")
                ctx.openPrefabRequest = path;
        }
        ImGui::EndDragDropTarget();
    }

    // Gizmo mode shortcuts (Unity-style W/E/R) while the viewport is hovered.
    if (m_hovered && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_W, false))      m_gizmoMode = GizmoMode::Translate;
        else if (ImGui::IsKeyPressed(ImGuiKey_E, false)) m_gizmoMode = GizmoMode::Rotate;
        else if (ImGui::IsKeyPressed(ImGuiKey_R, false)) m_gizmoMode = GizmoMode::Scale;
        else if (ImGui::IsKeyPressed(ImGuiKey_X, false))
            m_gizmoSpace = (m_gizmoSpace == GizmoSpace::World) ? GizmoSpace::Local : GizmoSpace::World;
    }

    // Tilemap paint mode takes input priority when active.
    const bool paintConsumed = !playing && paintTilemap(ctx);
    // Gizmo next — it may consume the mouse (so picking/camera don't also fire).
    const bool gizmoConsumed = !playing && !paintConsumed && drawGizmo(ctx);

    // Left-click (no drag) picks the entity under the cursor.
    if (!playing && !gizmoConsumed && !paintConsumed && ctx.scene && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mn = ImGui::GetItemRectMin();
        const ImVec2 mp = ImGui::GetMousePos();
        const glm::vec2 local{mp.x - mn.x, mp.y - mn.y};
        const Entity hit = Picking::pick(ctx.scene->world(), m_camera.viewProjection(),
                                         glm::vec2(static_cast<f32>(m_fbW), static_cast<f32>(m_fbH)), local);
        const ImGuiIO& io = ImGui::GetIO();
        if (io.KeyShift || io.KeyCtrl) { if (hit != NULL_ENTITY) ctx.selection.toggle(hit); }
        else                            ctx.selection.selectOnly(hit);   // selectOnly(NULL) clears
    }

    if (!playing && !gizmoConsumed && !paintConsumed) handleCameraInput();

    // Overlay: renderer stats (top-left of the image).
    const ImVec2 imgMin = ImGui::GetItemRectMin();
    const RendererStats& s = Renderer2D::stats();
    char buf[96];
    std::snprintf(buf, sizeof(buf), "draws %u  quads %u", s.drawCalls, s.quadCount);
    ImGui::GetWindowDrawList()->AddText(ImVec2(imgMin.x + 8.0f, imgMin.y + 6.0f),
                                        IM_COL32(220, 220, 220, 200), buf);

    drawEntityNames(ctx);   // world-space name labels (Show Entity Names overlay)

    ImGui::End();
    ImGui::PopStyleVar();
}

#ifdef EMBER_HAS_IMGUIZMO
namespace {
glm::mat4 trsOf(const Transform& t) {
    return glm::translate(glm::mat4(1.0f), t.position)
         * glm::rotate(glm::mat4(1.0f), t.rotation, glm::vec3(0, 0, 1))
         * glm::scale(glm::mat4(1.0f), t.scale);
}
void writeBackFromWorld(Transform& t, const glm::mat4& m) {
    float tr[3], rot[3], sc[3];
    ImGuizmo::DecomposeMatrixToComponents(&m[0][0], tr, rot, sc);
    t.position = { tr[0], tr[1], t.position.z };
    t.rotation = glm::radians(rot[2]);
    t.scale    = { sc[0], sc[1], t.scale.z };
}
} // namespace

bool ViewportPanel::drawGizmo(EditorContext& ctx) {
    if (!ctx.scene || ctx.selection.empty()) return false;
    World& w = ctx.scene->world();

    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist();
    const ImVec2 mn = ImGui::GetItemRectMin();
    ImGuizmo::SetRect(mn.x, mn.y, static_cast<float>(m_fbW), static_cast<float>(m_fbH));

    // Pivot at the selection's average position (recomputed each frame so it
    // follows the entities; ImGuizmo returns a per-frame delta we apply to each).
    glm::vec2 center{0.0f}; int n = 0;
    for (Entity e : ctx.selection.entities())
        if (const Transform* t = w.tryGet<Transform>(e)) { center += glm::vec2(t->position); ++n; }
    if (n == 0) return false;
    center /= static_cast<float>(n);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f));

    ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
    if (m_gizmoMode == GizmoMode::Rotate) op = ImGuizmo::ROTATE;
    else if (m_gizmoMode == GizmoMode::Scale) op = ImGuizmo::SCALE;
    const ImGuizmo::MODE md = (m_gizmoSpace == GizmoSpace::World) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;

    float snap[3] = {0, 0, 0};
    float* snapPtr = nullptr;
    if (ImGui::GetIO().KeyCtrl) {
        const float v = (op == ImGuizmo::ROTATE) ? m_snapAngle : m_snapGrid;
        snap[0] = snap[1] = snap[2] = v;
        snapPtr = snap;
    }

    glm::mat4 delta(1.0f);
    glm::mat4 view = m_camera.view();
    glm::mat4 proj = m_camera.projection();
    ImGuizmo::Manipulate(&view[0][0], &proj[0][0], op, md, &model[0][0], &delta[0][0], snapPtr);

    if (ImGuizmo::IsUsing()) {
        if (!m_gizmoUsing) {                       // drag start — capture before
            m_gizmoUsing = true;
            m_gizmoBefore.clear();
            for (Entity e : ctx.selection.entities())
                if (const Transform* t = w.tryGet<Transform>(e)) m_gizmoBefore.push_back({e, *t});
        }
        for (Entity e : ctx.selection.entities()) {
            if (Transform* t = w.tryGet<Transform>(e)) {
                writeBackFromWorld(*t, delta * trsOf(*t));   // apply this frame's delta
                ctx.scene->markTransformDirty(e);
            }
        }
        ctx.dirty = true;
    } else if (m_gizmoUsing) {                      // drag end — one undo step
        m_gizmoUsing = false;
        std::vector<Commands::EntityXform> after;
        for (const auto& b : m_gizmoBefore)
            if (const Transform* t = w.tryGet<Transform>(b.e)) after.push_back({b.e, *t});
        if (ctx.history)
            ctx.history->pushExecuted(Commands::transformEntities(*ctx.scene, m_gizmoBefore, after));
        m_gizmoBefore.clear();
    }

    return ImGuizmo::IsOver() || m_gizmoUsing;
}
#else
bool ViewportPanel::drawGizmo(EditorContext&) { return false; }
#endif

bool ViewportPanel::paintTilemap(EditorContext& ctx) {
    if (!ctx.tilemap.active || !ctx.scene) return false;
    const Entity e = ctx.selection.primary();
    Tilemap* tm = (e != NULL_ENTITY) ? ctx.scene->world().tryGet<Tilemap>(e) : nullptr;
    if (!tm) return false;
    if (!m_hovered) return true;   // in paint mode: still block camera/pick

    glm::vec2 origin{0.0f};
    if (const Transform* t = ctx.scene->world().tryGet<Transform>(e))
        origin = glm::vec2(t->position);

    const ImVec2 mn = ImGui::GetItemRectMin();
    const ImVec2 mp = ImGui::GetMousePos();
    const glm::vec2 local{mp.x - mn.x, mp.y - mn.y};
    const glm::vec3 wp = screenToWorld(local, m_camera.viewProjection(),
                                       glm::vec2(static_cast<f32>(m_fbW), static_cast<f32>(m_fbH)));
    u32 cx = 0, cy = 0;
    const bool inCell = TilemapPaint::worldToCell(*tm, origin, glm::vec2(wp.x, wp.y), cx, cy);

    const bool erase = (ctx.tilemap.tool == TilemapEditState::Tool::Erase);
    const u32  newId = erase ? 0u : ctx.tilemap.activeTile;

    if (ctx.tilemap.tool == TilemapEditState::Tool::Fill) {
        if (inCell && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            auto cells = TilemapPaint::floodFill(*tm, cx, cy, ctx.tilemap.activeTile);
            std::vector<Commands::TileEdit> edits;
            for (auto [fx, fy] : cells) {
                edits.push_back({fx, fy, tm->at(fx, fy), ctx.tilemap.activeTile});
                tm->setTile(fx, fy, ctx.tilemap.activeTile);
            }
            if (ctx.history && !edits.empty())
                ctx.history->pushExecuted(Commands::paintTiles(*ctx.scene, e, std::move(edits)));
            ctx.dirty = true;
        }
        return true;
    }

    // Paint / Erase: accumulate a click-drag stroke (one undo step).
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && inCell) {
        if (!m_painting) { m_painting = true; m_stroke.clear(); }
        const u32 old = tm->at(cx, cy);
        if (old != newId) {
            bool seen = false;
            for (const auto& ed : m_stroke) if (ed.x == cx && ed.y == cy) { seen = true; break; }
            if (!seen) { m_stroke.push_back({cx, cy, old, newId}); tm->setTile(cx, cy, newId); ctx.dirty = true; }
        }
    }
    if (m_painting && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (ctx.history && !m_stroke.empty())
            ctx.history->pushExecuted(Commands::paintTiles(*ctx.scene, e, m_stroke));
        m_stroke.clear();
        m_painting = false;
    }
    return true;
}

} // namespace ember
