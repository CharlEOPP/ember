#pragma once
#include "ember/scripting/ScriptComponent.h"
#include "ember/ecs/Reflect.h"
#include "ember/input/Input.h"

#include <glm/glm.hpp>

namespace game {

// WASD/arrow movement + Space to "jump" (no physics yet), driving the entity's
// Transform directly. Reads the Epic 05 Input facade. (SBX-01)
struct PlayerController : ember::ScriptComponent {
    float speed     = 5.0f;     // world units / second
    float jumpForce = 8.0f;     // applied as an upward velocity impulse

    void onUpdate(ember::f32 dt) override {
        using namespace ember;
        const f32 x = Input::getAction("MoveX").axisValue;   // -1..1
        const f32 y = Input::getAction("MoveY").axisValue;
        auto& t = transform();
        t.position.x += x * speed * dt;
        t.position.y += (y * speed + m_vy) * dt;

        if (Input::isKeyPressed(Key::Space) && !m_airborne) {
            m_vy = jumpForce;
            m_airborne = true;
        }
        m_vy -= 20.0f * dt;                 // gravity
        if (t.position.y <= 0.0f) { t.position.y = 0.0f; m_vy = 0.0f; m_airborne = false; }
    }

private:
    float m_vy = 0.0f;          // runtime-only
    bool  m_airborne = false;
};

EMBER_REFLECT(PlayerController) {
    EMBER_FIELD(speed);
    EMBER_FIELD(jumpForce);
}

} // namespace game
