#include "ember/scripting/ScriptComponent.h"
#include "ember/scripting/ScriptSystem.h"
#include "ember/assets/AssetManager.h"
#include "ember/assets/Prefab.h"
#include "ember/serialization/YAMLSerializer.h"
#include "ember/core/Log.h"

#include <glm/glm.hpp>

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

Entity ScriptComponent::instantiate(AssetHandle<Prefab> prefab) {
    AssetManager* assets = m_system ? m_system->assets() : nullptr;
    if (!assets || !m_scene) {
        EMBER_LOG_WARN("ScriptComponent::instantiate: no AssetManager set on the ScriptSystem "
                       "(call ScriptSystem::setAssetManager) — returning null entity");
        return NULL_ENTITY;
    }
    Prefab* p = assets->get<Prefab>(prefab);
    if (!p) {
        EMBER_LOG_WARN("ScriptComponent::instantiate: prefab handle not resident — null entity");
        return NULL_ENTITY;
    }
    auto result = YAMLDeserializer::instantiatePrefab(*m_scene, p->yaml, glm::vec3(0.0f));
    if (!result) {
        EMBER_LOG_ERROR("ScriptComponent::instantiate: {}", result.error());
        return NULL_ENTITY;
    }
    return result.value();
}

} // namespace ember
