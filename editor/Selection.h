#pragma once
#include "ember/ecs/Entity.h"

#include <type_traits>
#include <unordered_set>

namespace ember {

struct EntityHash {
    std::size_t operator()(Entity e) const noexcept {
        return std::hash<std::underlying_type_t<Entity>>{}(
            static_cast<std::underlying_type_t<Entity>>(e));
    }
};
using EntitySet = std::unordered_set<Entity, EntityHash>;

// Editor selection: a set of entities plus a "primary" (the active one — gizmo
// pivot / single-edit subject). ImGui-free, so it lives in ember_editor_core and
// is unit-testable. Single-select = a set of size 1 with primary == that entity.
class Selection {
public:
    [[nodiscard]] bool   empty()    const { return m_set.empty(); }
    [[nodiscard]] std::size_t size() const { return m_set.size(); }
    [[nodiscard]] bool   contains(Entity e) const { return m_set.count(e) != 0; }
    [[nodiscard]] Entity primary()  const { return m_primary; }   // NULL_ENTITY if empty
    [[nodiscard]] const EntitySet& entities() const { return m_set; }

    void clear() { m_set.clear(); m_primary = NULL_ENTITY; }

    void selectOnly(Entity e) {            // replace the whole selection
        m_set.clear();
        if (e == NULL_ENTITY) { m_primary = NULL_ENTITY; return; }
        m_set.insert(e);
        m_primary = e;
    }

    void add(Entity e) {                   // extend; e becomes primary
        if (e == NULL_ENTITY) return;
        m_set.insert(e);
        m_primary = e;
    }

    void remove(Entity e) {
        m_set.erase(e);
        if (m_primary == e)
            m_primary = m_set.empty() ? NULL_ENTITY : *m_set.begin();
    }

    void toggle(Entity e) {                // Shift/Ctrl-click semantics
        if (contains(e)) remove(e);
        else             add(e);
    }

private:
    EntitySet m_set;
    Entity    m_primary = NULL_ENTITY;
};

} // namespace ember
