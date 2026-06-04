#include "InspectorPanel.h"
#include "../InspectorRegistry.h"
#include "../Commands.h"
#include "../CommandHistory.h"

#include "ember/scene/Scene.h"
#include "ember/ecs/World.h"

#include <imgui.h>

namespace ember {

void InspectorPanel::onImGuiRender(EditorContext& ctx) {
    ImGui::Begin("Inspector");

    if (!ctx.scene || ctx.selected == NULL_ENTITY) {
        ImGui::TextUnformatted("No entity selected.");
        ImGui::End();
        return;
    }

    World&       w = ctx.scene->world();
    const Entity e = ctx.selected;

    for (const InspectorEntry& entry : InspectorRegistry::instance().all()) {
        if (!entry.has(w, e)) continue;
        ImGui::PushID(entry.name.c_str());

        const bool open = ImGui::CollapsingHeader(entry.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
        if (ImGui::BeginPopupContextItem("##comp_ctx")) {
            if (ImGui::MenuItem("Remove Component")) m_toRemove = &entry;
            ImGui::EndPopup();
        }

        if (open && entry.draw) {
            // Snapshot the pre-edit value so a change this frame can begin an
            // undoable, coalesced edit.
            std::shared_ptr<void> pre = entry.snapshot ? entry.snapshot(w, e) : nullptr;
            const bool changed = entry.draw(w, e);
            if (changed) {
                ctx.dirty = true;
                ctx.scene->markTransformDirty(e);
                if (!m_editing) {
                    m_editing    = true;
                    m_editEntry  = &entry;
                    m_editEntity = e;
                    m_editBefore = pre;
                }
            }
        }

        ImGui::PopID();
    }

    // Commit the coalesced edit once the user releases the widget.
    if (m_editing && !ImGui::IsAnyItemActive()) {
        if (ctx.history && m_editEntry && m_editEntry->snapshot && m_editBefore) {
            std::shared_ptr<void> after = m_editEntry->snapshot(w, m_editEntity);
            // Already applied live → record without re-executing.
            ctx.history->pushExecuted(
                Commands::setComponent(*ctx.scene, m_editEntity, *m_editEntry, m_editBefore, after));
        }
        m_editing    = false;
        m_editEntry  = nullptr;
        m_editBefore = nullptr;
    }

    ImGui::Separator();
    if (ImGui::Button("Add Component")) ImGui::OpenPopup("##add_component");
    if (ImGui::BeginPopup("##add_component")) {
        static char filter[64] = "";
        ImGui::InputText("filter", filter, sizeof(filter));
        for (const InspectorEntry& entry : InspectorRegistry::instance().all()) {
            if (entry.has(w, e)) continue;
            if (filter[0] && entry.name.find(filter) == std::string::npos) continue;
            if (ImGui::MenuItem(entry.name.c_str())) {
                World* wp = &w;
                auto add = entry.add, rem = entry.remove;
                if (ctx.history)
                    ctx.history->push(std::make_unique<FunctionalCommand>(
                        "Add " + entry.name,
                        [wp, e, add] { add(*wp, e); },
                        [wp, e, rem] { rem(*wp, e); }));
                else
                    entry.add(w, e);
                ctx.dirty = true;
            }
        }
        ImGui::EndPopup();
    }

    // Deferred removal (undoable: undo re-adds + restores the value).
    if (m_toRemove) {
        World* wp = &w;
        auto add     = m_toRemove->add;
        auto rem     = m_toRemove->remove;
        auto restore = m_toRemove->restore;
        std::shared_ptr<void> before = m_toRemove->snapshot ? m_toRemove->snapshot(w, e) : nullptr;
        const std::string nm = m_toRemove->name;
        if (ctx.history)
            ctx.history->push(std::make_unique<FunctionalCommand>(
                "Remove " + nm,
                [wp, e, rem] { rem(*wp, e); },
                [wp, e, add, restore, before] { add(*wp, e); if (restore && before) restore(*wp, e, before); }));
        else
            m_toRemove->remove(w, e);
        ctx.dirty = true;
        m_toRemove = nullptr;
    }

    ImGui::End();
}

} // namespace ember
