#pragma once
#include "../EditorContext.h"
#include "ember/ecs/Entity.h"

#include <memory>

namespace ember {

struct InspectorEntry;

// Shows the selected entity's components (each drawn by its registered inspector)
// plus Add/Remove Component. All mutations are routed through the CommandHistory;
// a continuous field drag coalesces into a single undo step.
class InspectorPanel {
public:
    void onImGuiRender(EditorContext& ctx);

private:
    const InspectorEntry* m_toRemove = nullptr;   // deferred removal

    // In-progress coalesced field edit.
    const InspectorEntry* m_editEntry  = nullptr;
    Entity                m_editEntity = NULL_ENTITY;
    std::shared_ptr<void> m_editBefore;
    bool                  m_editing = false;
};

} // namespace ember
