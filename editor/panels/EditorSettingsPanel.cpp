#include "EditorSettingsPanel.h"
#include "../EditorSettings.h"

#include <imgui.h>
#include <cstdio>

namespace ember {

bool EditorSettingsPanel::onImGuiRender(EditorSettings& s, bool& open) {
    if (!open) return false;

    bool changed = false;
    if (ImGui::Begin("Preferences", &open)) {
        changed |= ImGui::Combo("Theme", &s.theme, "Dark\0Light\0Classic\0");
        ImGui::SeparatorText("Gizmo snap");
        changed |= ImGui::DragFloat("Grid snap",   &s.gridSnap,   0.05f, 0.0f, 100.0f);
        changed |= ImGui::DragFloat("Rotate snap", &s.rotateSnap, 0.5f,  0.0f, 180.0f);
        ImGui::SeparatorText("Camera");
        changed |= ImGui::DragFloat("Pan speed",  &s.cameraPanSpeed,  0.05f, 0.05f, 10.0f);
        changed |= ImGui::DragFloat("Zoom speed", &s.cameraZoomSpeed, 0.05f, 0.05f, 10.0f);
        ImGui::SeparatorText("Workflow");
        changed |= ImGui::DragFloat("Auto-save (s, 0=off)", &s.autoSaveInterval, 1.0f, 0.0f, 600.0f);

        char tmpl[256];
        std::snprintf(tmpl, sizeof(tmpl), "%s", s.scriptTemplatePath.c_str());
        if (ImGui::InputText("Script template", tmpl, sizeof(tmpl))) {
            s.scriptTemplatePath = tmpl;
            changed = true;
        }
    }
    ImGui::End();
    return changed;
}

} // namespace ember
