#pragma once
//
// Internal serialization helpers. yaml-cpp meets the engine here and in the
// public <ember/serialization/ComponentSerialization.h> (which defines the
// field visitors + makeMeta, shared so other modules can register their own
// components). Not installed/public.
//
#include "ember/serialization/ComponentSerialization.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"
#include "ember/ecs/ComponentRegistry.h"
#include "ember/ecs/Reflect.h"
#include "ember/core/Log.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ember::detail {

// IdMap, idOf, YAMLWriteVisitor, YAMLReadVisitor and makeMeta<T> now live in
// <ember/serialization/ComponentSerialization.h> (included above).

inline void ensureRegistered() {
    auto& reg = ComponentRegistry::instance();
    if (reg.byName("Transform")) return;   // idempotent (also re-registers after a test clear())
    reg.registerComponent(makeMeta<Transform>("Transform"));
    reg.registerComponent(makeMeta<Tag>("Tag"));
    reg.registerComponent(makeMeta<Parent>("Parent"));
    reg.registerComponent(makeMeta<Children>("Children"));
}

// ---- Entity emit / load helpers (shared by scene + prefab) ----

inline void emitEntity(YAML::Emitter& out, World& world, Entity e) {
    out << YAML::BeginMap;
    out << YAML::Key << "id" << YAML::Value << idOf(e);
    if (const Tag* t = world.tryGet<Tag>(e))
        out << YAML::Key << "name" << YAML::Value << t->name;
    out << YAML::Key << "components" << YAML::Value << YAML::BeginMap;
    for (const auto& meta : ComponentRegistry::instance().all())
        if (meta.serialize) meta.serialize(world, e, &out);
    out << YAML::EndMap;   // components
    out << YAML::EndMap;   // entity
}

inline void collectSubtree(World& world, Entity e, std::vector<Entity>& out) {
    out.push_back(e);
    if (const Children* ch = world.tryGet<Children>(e))
        for (Entity c : ch->children) collectSubtree(world, c, out);
}

// Two-pass: create all entities (recording id remap), then populate components.
inline void loadEntities(World& world, const YAML::Node& entities, IdMap& idMap) {
    for (const auto& en : entities)
        idMap[en["id"].as<std::uint64_t>()] = world.create();

    for (const auto& en : entities) {
        Entity e = idMap[en["id"].as<std::uint64_t>()];
        if (auto comps = en["components"]; comps.IsDefined()) {
            for (auto it = comps.begin(); it != comps.end(); ++it) {
                const std::string cname = it->first.as<std::string>();
                const ComponentMeta* meta = ComponentRegistry::instance().byName(cname);
                if (!meta || !meta->deserialize) {
                    EMBER_LOG_WARN("Serialization: unknown component '{}' — skipping", cname);
                    continue;
                }
                YAML::Node cn = it->second;
                meta->deserialize(world, e, &cn, idMap);
            }
        }
        // Top-level "name" is the canonical entity name: ensure a Tag carries it
        // (covers files that give a name but no explicit Tag component).
        if (en["name"].IsDefined()) {
            Tag* tag = world.tryGet<Tag>(e);
            if (!tag) tag = &world.emplace<Tag>(e);
            tag->name = en["name"].as<std::string>();
        }
        if (!world.has<WorldTransform>(e)) world.emplace<WorldTransform>(e);
    }
}

} // namespace ember::detail
