#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Entity.h"

#include <functional>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ember {

class World;

// Metadata for one component type. The serialize/deserialize callbacks take void*
// (pointing at YAML::Emitter / YAML::Node) so yaml-cpp never appears in this header;
// they are populated by the serialization module, which owns the YAML dependency.
struct ComponentMeta {
    std::string     name;
    std::type_index type{typeid(void)};

    // Returns true and writes the component if the entity has it; false otherwise.
    std::function<bool(const World&, Entity, void* /*YAML::Emitter*/)> serialize;
    // Reads the component onto the entity; idMap remaps stored entity ids to live ones.
    std::function<void(World&, Entity, const void* /*YAML::Node*/,
                       const std::unordered_map<u64, Entity>& idMap)> deserialize;
    // Reserved for the editor inspector (Epic 07); null this epic.
    std::function<void(World&, Entity)> drawUI;
};

class ComponentRegistry {
public:
    static ComponentRegistry& instance();

    void registerComponent(ComponentMeta meta);                  // asserts on duplicate name
    [[nodiscard]] const ComponentMeta* byName(std::string_view name) const;
    [[nodiscard]] const ComponentMeta* byType(std::type_index type) const;
    [[nodiscard]] const std::vector<ComponentMeta>& all() const { return m_metas; }

    void clear();   // primarily for tests

private:
    std::vector<ComponentMeta>                  m_metas;
    std::unordered_map<std::string, usize>      m_byName;
    std::unordered_map<std::type_index, usize>  m_byType;
};

} // namespace ember
