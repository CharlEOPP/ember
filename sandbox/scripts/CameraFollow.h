#pragma once
#include "ember/scripting/ScriptComponent.h"
#include "ember/ecs/Reflect.h"
#include "ember/ecs/Components.h"

#include <glm/glm.hpp>

namespace game {

// Smoothly lerps its entity toward `target`'s position in onLateUpdate (runs
// after all movement that frame). Attach to a camera entity. (SBX-02)
struct CameraFollow : ember::ScriptComponent {
    ember::Entity target = ember::NULL_ENTITY;
    float         smoothing = 5.0f;   // higher = snappier

    void onLateUpdate(ember::f32 dt) override {
        using namespace ember;
        if (target == NULL_ENTITY) return;
        const Transform* tt = getWorld().tryGet<Transform>(target);
        if (!tt) return;
        auto& self = transform();
        const f32 a = glm::clamp(smoothing * dt, 0.0f, 1.0f);
        self.position += (tt->position - self.position) * a;
    }
};

EMBER_REFLECT(CameraFollow) {
    EMBER_FIELD(target);
    EMBER_FIELD(smoothing);
}

} // namespace game
