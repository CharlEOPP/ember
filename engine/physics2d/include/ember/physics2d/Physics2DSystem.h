#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/SystemScheduler.h"   // ISystem, World

#include <glm/glm.hpp>
#include <map>
#include <utility>
#include <vector>

namespace ember {

class ScriptSystem;

struct RaycastHit2D {
    bool      hit      = false;
    Entity    entity   = NULL_ENTITY;
    glm::vec2 point{0.0f};
    glm::vec2 normal{0.0f};
    f32       distance = 0.0f;
};

// Lightweight, dependency-free 2D physics: integrates RigidBody2D bodies on a
// fixed timestep, resolves Box/Circle collisions with impulses + positional
// correction, and answers ray/overlap queries. Collision *events* (script
// callbacks) are layered on in Phase 3. Runs on World directly (no Scene/GL).
class Physics2DSystem : public ISystem {
public:
    void update(World& world, f32 dt) override;   // accumulate dt, run fixed steps
    void step(World& world, f32 h);                // one fixed step (tests call directly)

    void setGravity(const glm::vec2& g) { m_gravity = g; }
    [[nodiscard]] glm::vec2 gravity() const { return m_gravity; }

    // When set, contact begin/stay/end are routed to script collision callbacks.
    void setScriptSystem(ScriptSystem* scripts) { m_scripts = scripts; }
    void setFixedDelta(f32 seconds) { m_fixedDelta = seconds; }
    [[nodiscard]] f32 fixedDelta() const { return m_fixedDelta; }

    // Queries against the current collider set in `world`.
    [[nodiscard]] RaycastHit2D raycast(World& world, const glm::vec2& origin,
                                       const glm::vec2& dir, f32 maxDist) const;
    [[nodiscard]] std::vector<Entity> overlapBox(World& world, const glm::vec2& center,
                                                 const glm::vec2& halfExtents) const;
    [[nodiscard]] std::vector<Entity> overlapCircle(World& world, const glm::vec2& center,
                                                    f32 radius) const;

private:
    struct ContactInfo { Entity a = NULL_ENTITY; Entity b = NULL_ENTITY; bool trigger = false; };

    glm::vec2 m_gravity{0.0f, -9.8f};
    f32 m_fixedDelta = 1.0f / 50.0f;   // 50 Hz (PHY-03)
    f32 m_accum      = 0.0f;
    ScriptSystem* m_scripts = nullptr;
    std::map<std::pair<u64, u64>, ContactInfo> m_prevContacts;   // for enter/stay/exit
};

} // namespace ember
