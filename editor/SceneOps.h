#pragma once
#include "ember/ecs/Entity.h"

#include <string>
#include <vector>

namespace ember {

class Scene;

// ImGui-free scene-mutation helpers shared by the hierarchy panel and (Phase 4)
// the command objects. Keeping them here makes them unit-testable headless.
namespace SceneOps {

Entity createEmpty (Scene& scene, const std::string& name = "Entity");
Entity createSprite(Scene& scene, const std::string& name = "Sprite");
Entity createCamera(Scene& scene, const std::string& name = "Camera");

// Deep-duplicate an entity sub-tree (via the prefab serialize/instantiate round
// trip); returns the new root (or NULL_ENTITY on failure).
Entity duplicate(Scene& scene, Entity e);

void rename(Scene& scene, Entity e, const std::string& name);

// Reparent `child` under `parent` (NULL_ENTITY = un-parent to root). Rejects
// self-parenting and cycles; returns true if applied.
bool reparent(Scene& scene, Entity child, Entity parent);

// True if `ancestor` is `e` or an ancestor of `e` (cycle guard).
bool isAncestorOrSelf(Scene& scene, Entity ancestor, Entity e);

// Top-level entities (no parent), in Tag-view order.
std::vector<Entity> roots(Scene& scene);

// Direct children of `e` (empty if none).
std::vector<Entity> childrenOf(Scene& scene, Entity e);

} // namespace SceneOps
} // namespace ember
