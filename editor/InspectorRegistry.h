#pragma once
#include "ember/ecs/Entity.h"
#include "ember/ecs/World.h"

#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>

namespace ember {

// One inspectable component type. has/add/remove/snapshot/restore are ImGui-free
// (testable); draw is supplied by the editor and uses ImGui. draw() returns true
// if a field was edited this frame. snapshot/restore copy the component value
// (type-erased) so the inspector can build undoable edit commands.
struct InspectorEntry {
    std::string                        name;
    std::type_index                    type{typeid(void)};
    std::function<bool(World&, Entity)> has;
    std::function<void(World&, Entity)> add;
    std::function<void(World&, Entity)> remove;
    std::function<bool(World&, Entity)> draw;
    std::function<std::shared_ptr<void>(World&, Entity)>                    snapshot;
    std::function<void(World&, Entity, const std::shared_ptr<void>&)>      restore;
};

// Editor-side table of inspectable components (separate from the engine's const
// ComponentRegistry; ImGui must not enter engine libraries).
class InspectorRegistry {
public:
    static InspectorRegistry& instance();

    void add(InspectorEntry e);                                  // dedupes by type
    [[nodiscard]] const std::vector<InspectorEntry>& all() const { return m_entries; }
    [[nodiscard]] const InspectorEntry* byType(std::type_index t) const;
    void clear();                                                // for tests

private:
    std::vector<InspectorEntry> m_entries;
};

// Register a component with default thunks; `draw` carries the (ImGui) field UI.
template<typename T>
void registerInspector(std::string name, std::function<bool(World&, Entity)> draw) {
    InspectorEntry e;
    e.name   = std::move(name);
    e.type   = std::type_index(typeid(T));
    e.has    = [](World& w, Entity ent) { return w.template tryGet<T>(ent) != nullptr; };
    e.add    = [](World& w, Entity ent) { if (!w.template tryGet<T>(ent)) w.template emplace<T>(ent); };
    e.remove = [](World& w, Entity ent) { if (w.template tryGet<T>(ent)) w.template remove<T>(ent); };
    e.draw   = std::move(draw);
    e.snapshot = [](World& w, Entity ent) -> std::shared_ptr<void> {
        return std::make_shared<T>(w.template get<T>(ent));
    };
    e.restore  = [](World& w, Entity ent, const std::shared_ptr<void>& s) {
        w.template get<T>(ent) = *static_cast<const T*>(s.get());
    };
    InspectorRegistry::instance().add(std::move(e));
}

} // namespace ember
