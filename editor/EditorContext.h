#pragma once
#include "ember/ecs/Entity.h"

namespace ember {

class Scene;
class AssetManager;
class CommandHistory;

// Shared editor state passed to each panel's onImGuiRender(). Kept small and
// ImGui-free so panel logic can be exercised without a UI context.
struct EditorContext {
    Scene*          scene    = nullptr;     // active edit scene
    AssetManager*   assets   = nullptr;
    CommandHistory* history  = nullptr;     // undo/redo
    Entity          selected = NULL_ENTITY; // single-select (NULL_ENTITY = none)
    bool            dirty    = false;       // unsaved changes
    bool            playing  = false;       // play mode active (panels read-only-ish)
};

} // namespace ember
