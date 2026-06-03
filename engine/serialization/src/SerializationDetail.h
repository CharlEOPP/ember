#pragma once
//
// Internal serialization helpers. This header is the ONLY place (besides the .cpp
// files that include it) where yaml-cpp meets the engine. Not installed/public.
//
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

using IdMap = std::unordered_map<u64, Entity>;

inline u64 idOf(Entity e) { return static_cast<u64>(entt::to_integral(e)); }

// ---- Field visitors ----

struct YAMLWriteVisitor {
    YAML::Emitter& out;

    void key(const char* n) { out << YAML::Key << n << YAML::Value; }

    void operator()(const char* n, f32& v)         { key(n); out << v; }
    void operator()(const char* n, i32& v)         { key(n); out << v; }
    void operator()(const char* n, u32& v)         { key(n); out << v; }
    void operator()(const char* n, u64& v)         { key(n); out << v; }
    void operator()(const char* n, bool& v)        { key(n); out << v; }
    void operator()(const char* n, std::string& v) { key(n); out << v; }
    void operator()(const char* n, glm::vec2& v)   { key(n); out << YAML::Flow << YAML::BeginSeq << v.x << v.y << YAML::EndSeq; }
    void operator()(const char* n, glm::vec3& v)   { key(n); out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq; }
    void operator()(const char* n, glm::vec4& v)   { key(n); out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq; }
    void operator()(const char* n, Entity& v)      { key(n); out << idOf(v); }
    void operator()(const char* n, std::vector<Entity>& v) {
        key(n);
        out << YAML::Flow << YAML::BeginSeq;
        for (Entity e : v) out << idOf(e);
        out << YAML::EndSeq;
    }
};

struct YAMLReadVisitor {
    const YAML::Node& node;
    const IdMap&      idMap;

    Entity remap(u64 id) const {
        auto it = idMap.find(id);
        return it == idMap.end() ? NULL_ENTITY : it->second;
    }

    void operator()(const char* n, f32& v)         { if (node[n].IsDefined()) v = node[n].as<float>(); }
    void operator()(const char* n, i32& v)         { if (node[n].IsDefined()) v = node[n].as<std::int32_t>(); }
    void operator()(const char* n, u32& v)         { if (node[n].IsDefined()) v = node[n].as<std::uint32_t>(); }
    void operator()(const char* n, u64& v)         { if (node[n].IsDefined()) v = node[n].as<std::uint64_t>(); }
    void operator()(const char* n, bool& v)        { if (node[n].IsDefined()) v = node[n].as<bool>(); }
    void operator()(const char* n, std::string& v) { if (node[n].IsDefined()) v = node[n].as<std::string>(); }
    void operator()(const char* n, glm::vec2& v)   { auto a = node[n]; if (a.IsDefined()) { v.x=a[0].as<float>(); v.y=a[1].as<float>(); } }
    void operator()(const char* n, glm::vec3& v)   { auto a = node[n]; if (a.IsDefined()) { v.x=a[0].as<float>(); v.y=a[1].as<float>(); v.z=a[2].as<float>(); } }
    void operator()(const char* n, glm::vec4& v)   { auto a = node[n]; if (a.IsDefined()) { v.x=a[0].as<float>(); v.y=a[1].as<float>(); v.z=a[2].as<float>(); v.w=a[3].as<float>(); } }
    void operator()(const char* n, Entity& v)      { if (node[n].IsDefined()) v = remap(node[n].as<std::uint64_t>()); }
    void operator()(const char* n, std::vector<Entity>& v) {
        auto a = node[n];
        v.clear();
        if (a.IsDefined()) for (auto el : a) v.push_back(remap(el.as<std::uint64_t>()));
    }
};

// ---- Component registration (the only place YAML + reflect + components meet) ----

template<typename T>
inline ComponentMeta makeMeta(const char* name) {
    ComponentMeta m;
    m.name = name;
    m.type = std::type_index(typeid(T));

    m.serialize = [name](const World& w, Entity e, void* emitterPtr) -> bool {
        if (!w.has<T>(e)) return false;
        auto& out = *static_cast<YAML::Emitter*>(emitterPtr);
        const T& comp = w.get<T>(e);
        out << YAML::Key << name << YAML::Value << YAML::BeginMap;
        YAMLWriteVisitor v{out};
        reflect(const_cast<T&>(comp), v);   // visitor only reads
        out << YAML::EndMap;
        return true;
    };

    m.deserialize = [](World& w, Entity e, const void* nodePtr, const IdMap& idMap) {
        const auto& node = *static_cast<const YAML::Node*>(nodePtr);
        T value{};
        YAMLReadVisitor v{node, idMap};
        reflect(value, v);
        w.emplace<T>(e, std::move(value));
    };

    return m;
}

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
