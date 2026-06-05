#pragma once
#include "../EditorContext.h"
#include "ember/ecs/Entity.h"

#include <memory>
#include <vector>

namespace ember {

struct InspectorEntry;

// Shows the selected entity's components (each drawn by its registered inspector)
// plus Add/Remove Component. All mutations are routed through the CommandHistory;
// a continuous field drag coalesces into a single undo step. In multi-select it
// shows components common to all selected and edits apply to the whole selection
// (one BatchSetField command).
class InspectorPanel {
public:
    void onImGuiRender(EditorContext& ctx);

private:
    const InspectorEntry* m_toRemove = nullptr;   // deferred removal

    // In-progress coalesced field edit (entities == {primary} when single-select).
    const InspectorEntry*              m_editEntry = nullptr;
    std::vector<Entity>                m_editEntities;
    std::vector<std::shared_ptr<void>> m_editBefore;
    bool                               m_editing = false;
};

} // namespace ember
