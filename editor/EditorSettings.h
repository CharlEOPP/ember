#pragma once
#include <string>

namespace ember {

// Editor preferences, persisted to a flat `key: value` file (no yaml-cpp in the
// editor). ImGui-free + in ember_editor_core so it's unit-testable.
struct EditorSettings {
    int   theme            = 0;      // 0 Dark, 1 Light, 2 Classic
    float gridSnap         = 1.0f;   // gizmo Ctrl-snap grid (world units)
    float rotateSnap       = 15.0f;  // gizmo Ctrl-snap angle (degrees)
    float cameraPanSpeed   = 1.0f;
    float cameraZoomSpeed  = 1.0f;
    float autoSaveInterval = 0.0f;   // seconds; 0 = off
    std::string scriptTemplatePath;

    // Returns false if the file was missing/unreadable (fields keep defaults).
    bool load(const std::string& path);
    bool save(const std::string& path) const;
};

} // namespace ember
