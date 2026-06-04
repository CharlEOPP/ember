#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/SystemScheduler.h"      // ISystem, World
#include "ember/scripting/ScriptComponent.h"

#include <entt/entity/registry.hpp>
#include <functional>
#include <vector>

namespace ember {

class Scene;

// Drives ScriptComponent lifecycle. One instance lives at Phase::Update. It
// builds a flat, order-sorted list of all registered scripts each tick and runs
// the start / fixed / update / late passes, plus deferred destruction. GPU- and
// yaml-free.
class ScriptSystem : public ISystem {
public:
    explicit ScriptSystem(Scene& scene);

    void update(World& world, f32 dt) override;

    // Called by ScriptComponent::destroy(); the entity is removed at end of frame.
    void scheduleDestroy(Entity e);

    void setFixedDelta(f32 seconds) { m_fixedDelta = seconds; }
    [[nodiscard]] f32 fixedDelta() const { return m_fixedDelta; }

    // entt on_destroy<T> target (static → usable as a free connect target). Fires
    // onDestroy before the component is removed, for both destroy() and external
    // world.destroy. Defined here so EMBER_REGISTER_SCRIPT can take its address.
    template<typename T>
    static void onComponentDestroyed(entt::registry& reg, entt::entity e) {
        fireDestroy(static_cast<ScriptComponent&>(reg.get<T>(e)));
    }

private:
    static void fireDestroy(ScriptComponent& s);     // guarded onDestroy
    // Run one callback with exception isolation; disables the script on throw (SS-08).
    static void guarded(ScriptComponent& s, const char* phase, const std::function<void()>& fn);
    void        inject(ScriptComponent& s, Entity e);

    struct ScriptEntry { Entity e; ScriptComponent* script; i32 order; const std::string* name; };

    Scene* m_scene = nullptr;
    f32    m_fixedDelta = 0.02f;   // 50 Hz (SS-06)
    f32    m_accum      = 0.0f;
    std::vector<Entity>      m_pendingDestroy;
    std::vector<ScriptEntry> m_list;   // reused each tick (NFR-02)
};

} // namespace ember
