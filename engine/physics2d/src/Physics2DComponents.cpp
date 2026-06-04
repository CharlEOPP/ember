#include "ember/physics2d/Physics2DComponents.h"
#include "ember/physics2d/RigidBody2D.h"
#include "ember/physics2d/Colliders2D.h"
#include "ember/serialization/ComponentSerialization.h"   // registerComponentType (yaml)

namespace ember {

void registerPhysics2DComponents() {
    registerComponentType<RigidBody2D>("RigidBody2D");
    registerComponentType<BoxCollider2D>("BoxCollider2D");
    registerComponentType<CircleCollider2D>("CircleCollider2D");
}

} // namespace ember
