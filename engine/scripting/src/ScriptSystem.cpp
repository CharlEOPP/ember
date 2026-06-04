#include "ember/scripting/ScriptSystem.h"
#include "ember/scripting/ScriptRegistry.h"
#include "ember/scene/Scene.h"
#include "ember/core/Log.h"

#include <algorithm>

namespace ember {

ScriptSystem::ScriptSystem(Scene& scene) : m_scene(&scene) {
    // Wire each registered script type's on_destroy hook so onDestroy fires
    // before removal (covers both destroy() and external world.destroy). Scripts
    // are registered at static-init, so all known types are present here.
    for (const auto& info : ScriptRegistry::instance().all())
        if (info.connectDestroy) info.connectDestroy(scene.world());
}

void ScriptSystem::inject(ScriptComponent& s, Entity e) {
    s.m_scene  = m_scene;
    s.m_entity = e;
    s.m_system = this;
}

void ScriptSystem::guarded(ScriptComponent& s, const char* phase, const std::function<void()>& fn) {
    if (!s.m_enabled) return;
    try {
        fn();
    } catch (const std::exception& ex) {
        EMBER_LOG_ERROR("ScriptSystem: script threw in {} ('{}') — disabling it", phase, ex.what());
        s.m_enabled = false;
    } catch (...) {
        EMBER_LOG_ERROR("ScriptSystem: script threw a non-std exception in {} — disabling it", phase);
        s.m_enabled = false;
    }
}

void ScriptSystem::fireDestroy(ScriptComponent& s) {
    if (!s.m_started) return;   // never started ⇒ nothing to tear down
    try {
        s.onDestroy();
    } catch (const std::exception& ex) {
        EMBER_LOG_ERROR("ScriptSystem: script threw in onDestroy ('{}')", ex.what());
    } catch (...) {
        EMBER_LOG_ERROR("ScriptSystem: script threw a non-std exception in onDestroy");
    }
}

void ScriptSystem::scheduleDestroy(Entity e) {
    m_pendingDestroy.push_back(e);
}

void ScriptSystem::update(World& world, f32 dt) {
    // 1. Rebuild the flat list from every registered script type's enumerator,
    //    injecting owner/scene, then sort by (order, name) for determinism.
    m_list.clear();
    for (const auto& info : ScriptRegistry::instance().all()) {
        if (!info.forEach) continue;
        info.forEach(world, [&](Entity e, ScriptComponent& s) {
            inject(s, e);
            m_list.push_back({ e, &s, info.order, &info.name });
        });
    }
    std::stable_sort(m_list.begin(), m_list.end(), [](const ScriptEntry& a, const ScriptEntry& b) {
        if (a.order != b.order) return a.order < b.order;
        return *a.name < *b.name;
    });

    // 2. onStart — once per script, the tick after it appears (SS-03).
    for (ScriptEntry& it : m_list) {
        if (it.script->m_started) continue;
        guarded(*it.script, "onStart", [&] { it.script->onStart(); });
        it.script->m_started = true;   // mark even on throw so it isn't retried
    }

    // 3. onFixedUpdate — fixed-timestep accumulator (SS-06), before onUpdate.
    m_accum += dt;
    while (m_accum >= m_fixedDelta) {
        for (ScriptEntry& it : m_list)
            if (it.script->m_started)
                guarded(*it.script, "onFixedUpdate", [&] { it.script->onFixedUpdate(m_fixedDelta); });
        m_accum -= m_fixedDelta;
    }

    // 4. onUpdate, then 5. onLateUpdate (SS-04/05).
    for (ScriptEntry& it : m_list)
        if (it.script->m_started)
            guarded(*it.script, "onUpdate", [&] { it.script->onUpdate(dt); });
    for (ScriptEntry& it : m_list)
        if (it.script->m_started)
            guarded(*it.script, "onLateUpdate", [&] { it.script->onLateUpdate(dt); });

    // 6. Drain deferred destroys; world.destroy fires the on_destroy hook →
    //    fireDestroy → onDestroy, before the entity is gone (SS-07).
    if (!m_pendingDestroy.empty()) {
        for (Entity e : m_pendingDestroy)
            if (world.valid(e)) world.destroy(e);
        m_pendingDestroy.clear();
    }
}

} // namespace ember
