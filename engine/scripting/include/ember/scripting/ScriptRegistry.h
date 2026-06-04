#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Entity.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace ember {

class World;
class ScriptComponent;

// Per-script-type metadata used by ScriptSystem. Populated by EMBER_REGISTER_SCRIPT
// (see RegisterScript.h). yaml-free on purpose so this header stays light.
struct ScriptTypeInfo {
    std::string name;
    i32         order = 0;   // lower runs earlier (ORD-01)

    // Enumerate live instances of this script type in a World.
    std::function<void(World&, const std::function<void(Entity, ScriptComponent&)>&)> forEach;
    // Connect this type's entt on_destroy<T> hook so onDestroy fires before removal.
    std::function<void(World&)> connectDestroy;
};

// Process-global table of registered script types (mirrors ComponentRegistry).
class ScriptRegistry {
public:
    static ScriptRegistry& instance();

    void registerScript(ScriptTypeInfo info);          // ignores duplicate names
    void setOrder(std::string_view name, i32 order);   // EMBER_SCRIPT_ORDER

    [[nodiscard]] const std::vector<ScriptTypeInfo>& all() const { return m_scripts; }
    [[nodiscard]] const ScriptTypeInfo* byName(std::string_view name) const;

    void clear();   // primarily for tests

private:
    std::vector<ScriptTypeInfo> m_scripts;
};

} // namespace ember
