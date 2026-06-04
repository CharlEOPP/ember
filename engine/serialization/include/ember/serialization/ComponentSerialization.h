#pragma once
//
// Component <-> YAML registration. This header pulls in yaml-cpp, so include it
// only from .cpp files that register component types (not from public engine
// headers). It lets any module register its own reflected components with the
// scene serializer without the serialization module needing to depend on that
// module (avoids a dependency cycle, e.g. renderer components).
//
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"
#include "ember/ecs/ComponentRegistry.h"
#include "ember/ecs/Reflect.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <string>
#include <typeindex>
#include <unordered_map>
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

// Build a ComponentMeta whose serialize/deserialize use reflection + YAML.
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

} // namespace ember::detail

namespace ember {

// Register a reflected component type `T` with the scene serializer under `name`.
// Idempotent-friendly: skips if a component with that name is already registered.
template<typename T>
inline void registerComponentType(const char* name) {
    auto& reg = ComponentRegistry::instance();
    if (reg.byName(name)) return;
    reg.registerComponent(detail::makeMeta<T>(name));
}

} // namespace ember
