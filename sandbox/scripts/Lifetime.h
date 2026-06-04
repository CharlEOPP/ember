#pragma once
#include "ember/scripting/ScriptComponent.h"
#include "ember/ecs/Reflect.h"

namespace game {

// Destroys its entity after `duration` seconds, proving deferred destroy +
// onDestroy. (SBX-04)
struct Lifetime : ember::ScriptComponent {
    float duration = 1.0f;

    void onUpdate(ember::f32 dt) override {
        m_elapsed += dt;
        if (m_elapsed >= duration) destroy();
    }

private:
    float m_elapsed = 0.0f;   // runtime-only (not reflected)
};

EMBER_REFLECT(Lifetime) { EMBER_FIELD(duration); }

} // namespace game
