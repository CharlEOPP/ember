#include "InspectorPanel.h"
#include "../InspectorRegistry.h"
#include "../Commands.h"
#include "../CommandHistory.h"

#include "ember/scene/Scene.h"
#include "ember/scene/PrefabInstance.h"
#include "ember/ecs/World.h"

#include <imgui.h>

namespace ember {

void InspectorPanel::onImGuiRender(EditorContext& ctx) {
    ImGui::Begin("Inspector");

    if (!ctx.scene || ctx.selection.empty()) {
        ImGui::TextUnformatted("No entity selected.");
        ImGui::End();
        return;
    }

    World&       w       = ctx.scene->world();
    const Entity primary = ctx.selection.primary();
    const std::vector<Entity> sel(ctx.selection.entities().begin(), ctx.selection.entities().end());
    const bool   multi   = sel.size() > 1;

    if (multi) ImGui::Text("%zu entities selected", sel.size());

    // Prefab instance banner (single-select).
    if (!multi) {
        if (const PrefabInstance* pi = w.tryGet<PrefabInstance>(primary)) {
            ImGui::TextColored(ImVec4(0.47f, 0.67f, 1.0f, 1.0f), "Prefab: %s", pi->source.c_str());
            if (ImGui::Button("Revert to Prefab")) ctx.revertPrefabRequest = primary;
            ImGui::Separator();
        }
    }

    for (const InspectorEntry& entry : InspectorRegistry::instance().all()) {
        if (!entry.has(w, primary)) continue;
        if (multi) {                               // only components common to ALL selected
            bool all = true;
            for (Entity e : sel) if (!entry.has(w, e)) { all = false; break; }
            if (!all) continue;
        }
        ImGui::PushID(entry.name.c_str());

        const bool open = ImGui::CollapsingHeader(entry.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
        if (ImGui::BeginPopupContextItem("##comp_ctx")) {
            if (ImGui::MenuItem("Remove Component")) m_toRemove = &entry;
            ImGui::EndPopup();
        }

        if (open && entry.draw) {
            std::shared_ptr<void> pre = entry.snapshot ? entry.snapshot(w, primary) : nullptr;
            const bool changed = entry.draw(w, primary);   // edits the primary
            if (changed) {
                ctx.dirty = true;
                ctx.scene->markTransformDirty(primary);

                if (!m_editing) {                  // capture pre-edit state for all targets
                    m_editing    = true;
                    m_editEntry  = &entry;
                    m_editEntities = multi ? sel : std::vector<Entity>{primary};
                    m_editBefore.clear();
                    for (Entity e : m_editEntities)
                        m_editBefore.push_back(e == primary ? pre
                                                            : (entry.snapshot ? entry.snapshot(w, e) : nullptr));
                }
                if (multi && entry.snapshot && entry.restore) {   // sync primary's value to others live
                    std::shared_ptr<void> cur = entry.snapshot(w, primary);
                    for (Entity e : sel)
                        if (e != primary) { entry.restore(w, e, cur); ctx.scene->markTransformDirty(e); }
                }
            }
        }
        ImGui::PopID();
    }

    // Commit the coalesced edit when the widget is released.
    if (m_editing && !ImGui::IsAnyItemActive()) {
        if (ctx.history && m_editEntry && m_editEntry->snapshot) {
            std::vector<std::shared_ptr<void>> after;
            for (Entity e : m_editEntities) after.push_back(m_editEntry->snapshot(w, e));
            ctx.history->pushExecuted(
                Commands::setComponentBatch(*ctx.scene, *m_editEntry, m_editEntities, m_editBefore, after));
        }
        m_editing = false;
        m_editEntry = nullptr;
        m_editEntities.clear();
        m_editBefore.clear();
    }

    ImGui::Separator();
    if (ImGui::Button("Add Component")) ImGui::OpenPopup("##add_component");
    if (ImGui::BeginPopup("##add_component")) {
        static char filter[64] = "";
        ImGui::InputText("filter", filter, sizeof(filter));
        for (const InspectorEntry& entry : InspectorRegistry::instance().all()) {
            if (entry.has(w, primary)) continue;
            if (filter[0] && entry.name.find(filter) == std::string::npos) continue;
            if (ImGui::MenuItem(entry.name.c_str())) {
                World* wp = &w;
                auto add = entry.add, rem = entry.remove;
                std::vector<Entity> targets = sel;   // add to every selected entity
                if (ctx.history)
                    ctx.history->push(std::make_unique<FunctionalCommand>(
                        "Add " + entry.name,
                        [wp, targets, add] { for (Entity e : targets) add(*wp, e); },
                        [wp, targets, rem] { for (Entity e : targets) rem(*wp, e); }));
                else
                    for (Entity e : targets) entry.add(w, e);
                ctx.dirty = true;
            }
        }
        ImGui::EndPopup();
    }

    // Deferred removal (undoable: undo re-adds + restores per-entity value).
    if (m_toRemove) {
        World* wp = &w;
        auto add      = m_toRemove->add;
        auto rem      = m_toRemove->remove;
        auto restore  = m_toRemove->restore;
        auto snapshot = m_toRemove->snapshot;
        std::vector<Entity> targets;
        for (Entity e : sel) if (m_toRemove->has(w, e)) targets.push_back(e);

        std::vector<std::shared_ptr<void>> before;
        for (Entity e : targets) before.push_back(snapshot ? snapshot(w, e) : nullptr);

        const std::string nm = m_toRemove->name;
        if (ctx.history)
            ctx.history->push(std::make_unique<FunctionalCommand>(
                "Remove " + nm,
                [wp, targets, rem] { for (Entity e : targets) rem(*wp, e); },
                [wp, targets, add, restore, before] {
                    for (std::size_t i = 0; i < targets.size(); ++i) {
                        add(*wp, targets[i]);
                        if (restore && before[i]) restore(*wp, targets[i], before[i]);
                    }
                }));
        else
            for (Entity e : targets) rem(*wp, e);
        ctx.dirty = true;
        m_toRemove = nullptr;
    }

    ImGui::End();
}

} // namespace ember
