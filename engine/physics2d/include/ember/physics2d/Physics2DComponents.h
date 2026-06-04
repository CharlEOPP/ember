#pragma once

namespace ember {

// Register RigidBody2D / BoxCollider2D / CircleCollider2D with the scene
// serializer's ComponentRegistry. Call once at startup. Idempotent.
void registerPhysics2DComponents();

} // namespace ember
