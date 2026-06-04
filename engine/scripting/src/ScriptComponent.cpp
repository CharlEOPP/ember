#include "ember/scripting/ScriptComponent.h"
#include "ember/scripting/ScriptSystem.h"
#include "ember/core/Log.h"

namespace ember {

const std::string& ScriptComponent::getName() const {
    static const std::string kEmpty;
    if (const Tag* t = m_scene->world().tryGet<Tag>(m_entity)) return t->name;
    return kEmpty;
}

void ScriptComponent::setName(const std::string& name) {
    World& w = m_scene->world();
    Tag* t = w.tryGet<Tag>(m_entity);
    if (!t) t = &w.emplace<Tag>(m_entity);
    t->name = name;
}

void ScriptComponent::destroy() {
    if (m_system) m_system->scheduleDestroy(m_entity);
}

Entity ScriptComponent::instantiate(AssetHandle<Prefab> /*prefab*/) {
    EMBER_LOG_WARN("ScriptComponent::instantiate: Prefab asset type not implemented yet "
                   "(Epic 06 §11) — returning null entity");
    return NULL_ENTITY;
}

} // namespace ember
