#include "TilemapEditorPanel.h"

#include "ember/scene/Scene.h"
#include "ember/ecs/World.h"
#include "ember/renderer/Tilemap.h"

#include <imgui.h>
#include <cstdio>

namespace ember {

void TilemapEditorPanel::onImGuiRender(EditorContext& ctx) {
    ImGui::Begin("Tilemap");

    Tilemap* tm = nullptr;
    if (ctx.scene && ctx.selection.primary() != NULL_ENTITY)
        tm = ctx.scene->world().tryGet<Tilemap>(ctx.selection.primary());

    if (!tm) {
        ImGui::TextUnformatted("Select an entity with a Tilemap component.");
        ctx.tilemap.active = false;
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Paint mode", &ctx.tilemap.active);
    ImGui::Text("Grid: %u x %u", tm->width, tm->height);

    // Tool selection.
    int tool = static_cast<int>(ctx.tilemap.tool);
    ImGui::RadioButton("Paint", &tool, 0); ImGui::SameLine();
    ImGui::RadioButton("Erase", &tool, 1); ImGui::SameLine();
    ImGui::RadioButton("Fill",  &tool, 2);
    ctx.tilemap.tool = static_cast<TilemapEditState::Tool>(tool);

    ImGui::Separator();
    ImGui::Text("Palette (active tile: %u)", ctx.tilemap.activeTile);

    // Palette: numbered buttons for each atlas cell (id 1..columns*rows).
    const unsigned count = tm->tileset.columns * tm->tileset.rows;
    const unsigned cols  = tm->tileset.columns ? tm->tileset.columns : 1u;
    for (unsigned id = 1; id <= count; ++id) {
        char label[16];
        std::snprintf(label, sizeof(label), "%u", id);
        const bool selected = (ctx.tilemap.activeTile == id);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.50f, 0.80f, 1.0f));
        if (ImGui::Button(label, ImVec2(32, 32))) ctx.tilemap.activeTile = id;
        if (selected) ImGui::PopStyleColor();
        if (id % cols != 0 && id != count) ImGui::SameLine();
    }

    ImGui::End();
}

} // namespace ember
