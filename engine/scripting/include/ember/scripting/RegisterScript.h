#pragma once
//
// Script registration. Include this only from the (game) translation unit that
// registers script types — it pulls in yaml-cpp via the serialization header.
// ScriptComponent.h / ScriptSystem.h stay yaml-free for everyone else.
//
#include "ember/scripting/ScriptComponent.h"
#include "ember/scripting/ScriptRegistry.h"
#include "ember/scripting/ScriptSystem.h"
#include "ember/ecs/World.h"
#include "ember/serialization/ComponentSerialization.h"   // registerComponentType<T> (yaml)

#include <functional>
#include <type_traits>

namespace ember {

namespace detail {
template<typename T>
inline ScriptTypeInfo makeScriptInfo(const char* name, i32 order) {
    static_assert(std::is_base_of_v<ScriptComponent, T>, "T must derive from ScriptComponent");
    ScriptTypeInfo info;
    info.name  = name;
    info.order = order;
    info.forEach = [](World& w, const std::function<void(Entity, ScriptComponent&)>& cb) {
        for (auto&& [e, c] : w.view<T>()) cb(e, static_cast<ScriptComponent&>(c));
    };
    info.connectDestroy = [](World& w) {
        w.template on_destroy<T>().template connect<&ScriptSystem::onComponentDestroyed<T>>();
    };
    info.tryDispatch = [](World& w, Entity e, const std::function<void(ScriptComponent&)>& fn) {
        if (auto* c = w.tryGet<T>(e)) fn(static_cast<ScriptComponent&>(*c));
    };
    return info;
}
} // namespace detail

// Register a script type: ScriptRegistry + scene serialization (reflected fields).
template<typename T>
inline void registerScriptType(const char* name, i32 order = 0) {
    ScriptRegistry::instance().registerScript(detail::makeScriptInfo<T>(name, order));
    registerComponentType<T>(name);   // REG-01: serializes EMBER_FIELD members
}

// Register a script with no reflected/serialized fields (REG-04).
template<typename T>
inline void registerScriptTypeNoSerialize(const char* name, i32 order = 0) {
    ScriptRegistry::instance().registerScript(detail::makeScriptInfo<T>(name, order));
}

} // namespace ember

// In a game .cpp: EMBER_REGISTER_SCRIPT(PlayerController)
#define EMBER_REGISTER_SCRIPT(Type) \
    namespace { const bool ember_script_reg_##Type = \
        (::ember::registerScriptType<Type>(#Type), true); }

#define EMBER_REGISTER_SCRIPT_NOSERIALIZE(Type) \
    namespace { const bool ember_script_reg_##Type = \
        (::ember::registerScriptTypeNoSerialize<Type>(#Type), true); }

// Place AFTER EMBER_REGISTER_SCRIPT(Type) in the same TU.
#define EMBER_SCRIPT_ORDER(Type, n) \
    namespace { const bool ember_script_order_##Type = \
        (::ember::ScriptRegistry::instance().setOrder(#Type, n), true); }
