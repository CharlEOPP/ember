#pragma once
#include "ember/ecs/Entity.h"
#include "../EditorContext.h"

namespace ember {

// Tree view of the active scene: select, create, duplicate, delete, reparent
// (drag-drop), and inline rename. Mutations go through SceneOps (Phase 2) and are
// rerouted through the CommandHistory in Phase 4.
class SceneHierarchyPanel {
public:
    void onImGuiRender(EditorContext& ctx);

private:
    void drawNode(EditorContext& ctx, Entity e);

    // Deferred actions (applied after the tree walk to keep iteration valid).
    enum class Pending { None, CreateEmpty, CreateSprite, CreateCamera, Duplicate, Delete };
    Pending m_pending       = Pending::None;
    Entity  m_pendingTarget = NULL_ENTITY;     // target for Duplicate/Delete

    Entity  m_reparentChild  = NULL_ENTITY;    // pending reparent (child -> parent)
    Entity  m_reparentParent = NULL_ENTITY;
    bool    m_hasReparent    = false;

    Entity  m_renaming = NULL_ENTITY;          // entity being renamed inline
    char    m_renameBuf[128] = {0};
};

} // namespace ember
