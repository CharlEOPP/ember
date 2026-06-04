#pragma once
#include "ember/scripting/ScriptComponent.h"
#include "ember/ecs/Reflect.h"

namespace game {

// Rotates its entity's Transform at `speed` radians/second. (SBX-03)
struct Spinner : ember::ScriptComponent {
    float speed = 1.0f;
    void onUpdate(ember::f32 dt) override { transform().rotation += speed * dt; }
};

EMBER_REFLECT(Spinner) { EMBER_FIELD(speed); }

} // namespace game
