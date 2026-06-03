#include "ember/ecs/ComponentRegistry.h"
#include "ember/core/Assert.h"

#include <utility>

namespace ember {

ComponentRegistry& ComponentRegistry::instance() {
    static ComponentRegistry s_instance;
    return s_instance;
}

void ComponentRegistry::registerComponent(ComponentMeta meta) {
    EMBER_ASSERT(m_byName.find(meta.name) == m_byName.end(),
                 "ComponentRegistry: component '{}' already registered", meta.name);
    if (m_byName.find(meta.name) != m_byName.end()) return;  // no duplicates

    const usize idx = m_metas.size();
    m_byName.emplace(meta.name, idx);
    m_byType.emplace(meta.type, idx);
    m_metas.push_back(std::move(meta));
}

const ComponentMeta* ComponentRegistry::byName(std::string_view name) const {
    auto it = m_byName.find(std::string(name));
    return it == m_byName.end() ? nullptr : &m_metas[it->second];
}

const ComponentMeta* ComponentRegistry::byType(std::type_index type) const {
    auto it = m_byType.find(type);
    return it == m_byType.end() ? nullptr : &m_metas[it->second];
}

void ComponentRegistry::clear() {
    m_metas.clear();
    m_byName.clear();
    m_byType.clear();
}

} // namespace ember
