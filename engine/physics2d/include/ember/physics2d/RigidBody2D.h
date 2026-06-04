#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Reflect.h"
#include <glm/glm.hpp>

namespace ember {

enum class BodyType2D : u8 {
    Static,     // never moves; infinite mass
    Kinematic,  // moves by velocity, not pushed by collisions
    Dynamic,    // full simulation
};

// 2D rigid body. Attach with a collider (BoxCollider2D / CircleCollider2D) for
// it to participate in collisions. `velocity` is in world units/sec.
struct RigidBody2D {
    BodyType2D type          = BodyType2D::Dynamic;
    glm::vec2  velocity      {0.0f};
    f32        gravityScale  = 1.0f;
    f32        mass          = 1.0f;
    f32        linearDamping = 0.0f;
    bool       fixedRotation = true;   // 2D sprites: no angular dynamics this epic

    // Runtime-only accumulator (not serialized); applied + cleared each step.
    glm::vec2  force         {0.0f};

    // ---- Script API ----
    void applyForce(const glm::vec2& f)   { force += f; }
    void applyImpulse(const glm::vec2& j) { if (mass > 0.0f && type == BodyType2D::Dynamic) velocity += j / mass; }
    void setVelocity(const glm::vec2& v)  { velocity = v; }
    [[nodiscard]] glm::vec2 getVelocity() const { return velocity; }

    [[nodiscard]] f32 invMass() const {
        return (type == BodyType2D::Dynamic && mass > 0.0f) ? 1.0f / mass : 0.0f;
    }
};

EMBER_REFLECT(RigidBody2D) {
    EMBER_FIELD(type);
    EMBER_FIELD(velocity);
    EMBER_FIELD(gravityScale);
    EMBER_FIELD(mass);
    EMBER_FIELD(linearDamping);
    EMBER_FIELD(fixedRotation);
}

} // namespace ember
