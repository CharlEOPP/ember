#include "SceneHierarchyPanel.h"
#include "../SceneOps.h"
#include "../Commands.h"
#include "../CommandHistory.h"

#include "ember/scene/Scene.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"

#include <imgui.h>
#include <cstring>
#include <memory>
#include <type_traits>

namespace ember {

static const char* entityLabel(Scene& scene, Entity e, char* buf, size_t n) {
    const Tag* tag = scene.world().tryGet<Tag>(e);
    if (tag && !tag->name.empty()) return tag->name.c_str();
    std::snprintf(buf, n, "Entity %llu",
                  static_cast<unsigned long long>(static_cast<std::underlying_type_t<Entity>>(e)));
    return buf;
}

void SceneHierarchyPanel::drawNode(EditorContext& ctx, Entity e) {
    Scene& scene = *ctx.scene;
    const std::vector<Entity> children = SceneOps::childrenOf(scene, e);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (children.empty())     flags |= ImGuiTreeNodeFlags_Leaf;
    if (ctx.selected == e)    flags |= ImGuiTreeNodeFlags_Selected;

    char tmp[64];
    const void* id = reinterpret_cast<void*>(static_cast<uintptr_t>(
        static_cast<std::underlying_type_t<Entity>>(e)));

    bool open;
    if (m_renaming == e) {
        // Inline rename: tree node label replaced by an input field.
        ImGui::TreePush(id);
        ImGui::SetKeyboardFocusHere();
        if (ImGui::InputText("##rename", m_renameBuf, sizeof(m_renameBuf),
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)
            || ImGui::IsItemDeactivated()) {
            const Tag* tag = scene.world().tryGet<Tag>(e);
            const std::string oldName = tag ? tag->name : std::string{};
            if (oldName != m_renameBuf) {
                if (ctx.history) ctx.history->push(Commands::rename(scene, e, oldName, m_renameBuf));
                else             SceneOps::rename(scene, e, m_renameBuf);
                ctx.dirty = true;
            }
            m_renaming = NULL_ENTITY;
        }
        open = true;   // keep expanded while editing
    } else {
        open = ImGui::TreeNodeEx(id, flags, "%s", entityLabel(scene, e, tmp, sizeof(tmp)));
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            ctx.selected = e;
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) {
            m_renaming = e;
            const char* lbl = entityLabel(scene, e, tmp, sizeof(tmp));
            std::snprintf(m_renameBuf, sizeof(m_renameBuf), "%s", lbl);
        }
    }

    // Drag source / drop target for reparenting.
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("EMBER_ENTITY", &e, sizeof(Entity));
        ImGui::TextUnformatted(entityLabel(scene, e, tmp, sizeof(tmp)));
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("EMBER_ENTITY")) {
            m_reparentChild  = *static_cast<const Entity*>(p->Data);
            m_reparentParent = e;
            m_hasReparent    = true;
        }
        ImGui::EndDragDropTarget();
    }

    // Per-entity context menu.
    if (ImGui::BeginPopupContextItem()) {
        ctx.selected = e;
        if (ImGui::MenuItem("Create Empty"))  { m_pending = Pending::CreateEmpty;  m_pendingTarget = e; }
        if (ImGui::MenuItem("Create Sprite")) { m_pending = Pending::CreateSprite; m_pendingTarget = e; }
        if (ImGui::MenuItem("Create Camera")) { m_pending = Pending::CreateCamera; m_pendingTarget = e; }
        ImGui::Separator();
        if (ImGui::MenuItem("Duplicate"))     { m_pending = Pending::Duplicate;    m_pendingTarget = e; }
        if (ImGui::MenuItem("Delete"))        { m_pending = Pending::Delete;       m_pendingTarget = e; }
        ImGui::EndPopup();
    }

    if (open) {
        for (Entity c : children) drawNode(ctx, c);
        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::onImGuiRender(EditorContext& ctx) {
    ImGui::Begin("Hierarchy");

    if (ctx.scene) {
        Scene& scene = *ctx.scene;

        for (Entity root : SceneOps::roots(scene)) drawNode(ctx, root);

        // Click empty space clears selection.
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && !ImGui::IsAnyItemHovered())
            ctx.selected = NULL_ENTITY;

        // Drop onto the panel background = un-parent.
        ImGui::Dummy(ImGui::GetContentRegionAvail());
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("EMBER_ENTITY")) {
                m_reparentChild  = *static_cast<const Entity*>(p->Data);
                m_reparentParent = NULL_ENTITY;
                m_hasReparent    = true;
            }
            ImGui::EndDragDropTarget();
        }

        // Background context menu (create at root).
        if (ImGui::BeginPopupContextWindow("##hier_ctx", ImGuiPopupFlags_MouseButtonRight |
                                                          ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("Create Empty"))  { m_pending = Pending::CreateEmpty;  m_pendingTarget = NULL_ENTITY; }
            if (ImGui::MenuItem("Create Sprite")) { m_pending = Pending::CreateSprite; m_pendingTarget = NULL_ENTITY; }
            if (ImGui::MenuItem("Create Camera")) { m_pending = Pending::CreateCamera; m_pendingTarget = NULL_ENTITY; }
            ImGui::EndPopup();
        }

        // ---- Apply deferred actions (after the tree walk) ----
        if (m_hasReparent) {
            const Entity oldParent = scene.getParent(m_reparentChild);
            if (ctx.history)
                ctx.history->push(Commands::reparent(scene, m_reparentChild, oldParent, m_reparentParent));
            else
                SceneOps::reparent(scene, m_reparentChild, m_reparentParent);
            ctx.dirty = true;
            m_hasReparent = false;
        }

        if (m_pending != Pending::None) {
            using CK = Commands::CreateKind;
            auto created = std::make_shared<Entity>(NULL_ENTITY);
            switch (m_pending) {
                case Pending::CreateEmpty:
                    if (ctx.history) ctx.history->push(Commands::create(scene, CK::Empty, m_pendingTarget, created));
                    break;
                case Pending::CreateSprite:
                    if (ctx.history) ctx.history->push(Commands::create(scene, CK::Sprite, m_pendingTarget, created));
                    break;
                case Pending::CreateCamera:
                    if (ctx.history) ctx.history->push(Commands::create(scene, CK::Camera, m_pendingTarget, created));
                    break;
                case Pending::Duplicate:
                    if (ctx.history) ctx.history->push(Commands::duplicate(scene, m_pendingTarget, created));
                    break;
                case Pending::Delete:
                    if (ctx.selected == m_pendingTarget) ctx.selected = NULL_ENTITY;
                    if (ctx.history) ctx.history->push(Commands::remove(scene, m_pendingTarget));
                    else             scene.destroy(m_pendingTarget);
                    break;
                case Pending::None: break;
            }
            if (*created != NULL_ENTITY) ctx.selected = *created;
            ctx.dirty   = true;
            m_pending   = Pending::None;
        }
    }

    ImGui::End();
}

} // namespace ember
