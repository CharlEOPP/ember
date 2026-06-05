#pragma once
#include "ember/ecs/Entity.h"
#include "Selection.h"

#include <string>

namespace ember {

class Scene;
class AssetManager;
class CommandHistory;

// Editor-only debug overlay toggles (never serialized in the scene).
struct DebugOverlays {
    bool  grid      = false;
    bool  names     = false;
    bool  colliders = false;
    bool  frustums  = false;
    float gridCell  = 1.0f;
};

// Shared tilemap-paint state: the panel sets it, the viewport paints with it.
struct TilemapEditState {
    enum class Tool { Paint, Erase, Fill };
    bool active     = false;     // viewport paint mode on
    Tool tool       = Tool::Paint;
    unsigned activeTile = 1;     // tile id to paint
};

// Shared editor state passed to each panel's onImGuiRender(). Kept small and
// ImGui-free so panel logic can be exercised without a UI context.
struct EditorContext {
    Scene*          scene    = nullptr;     // active edit scene
    AssetManager*   assets   = nullptr;
    CommandHistory* history  = nullptr;     // undo/redo
    Selection       selection;              // selected entities + primary
    bool            dirty    = false;       // unsaved changes
    bool            playing  = false;       // play mode active (panels read-only-ish)

    // Cross-panel requests, consumed by EditorApplication each frame.
    Entity          savePrefabRequest   = NULL_ENTITY; // entity to "Save as Prefab"
    std::string     openPrefabRequest;                 // .eprefab path to instantiate
    Entity          revertPrefabRequest = NULL_ENTITY; // instance to revert to its source

    TilemapEditState tilemap;                          // tilemap paint mode (Phase 5)
    DebugOverlays    overlays;                          // debug overlays (Phase 8)
};

} // namespace ember
