#pragma once
#include "CommandHistory.h"
#include "InspectorRegistry.h"
#include "ember/ecs/Entity.h"

#include <memory>
#include <string>

namespace ember {

class Scene;

// Built-in editor commands (each returns a Command ready to push). All capture
// the Scene by pointer (it outlives the history) and restore prior state on undo.
namespace Commands {

enum class CreateKind { Empty, Sprite, Camera };

// Creates an entity (optionally under `parent`); `created` receives the new id
// after the command is pushed. Undo destroys it.
std::unique_ptr<Command> create(Scene& scene, CreateKind kind, Entity parent,
                                std::shared_ptr<Entity> created);

// Deletes a sub-tree (serialized to a prefab string so undo can re-instantiate it).
std::unique_ptr<Command> remove(Scene& scene, Entity e);

// Deep-duplicates a sub-tree; `created` receives the new root id.
std::unique_ptr<Command> duplicate(Scene& scene, Entity src, std::shared_ptr<Entity> created);

std::unique_ptr<Command> reparent(Scene& scene, Entity child, Entity oldParent, Entity newParent);
std::unique_ptr<Command> rename(Scene& scene, Entity e, std::string oldName, std::string newName);

// Component-field edit: restores `before`/`after` snapshots produced by the
// InspectorEntry's snapshot()/restore() thunks (already applied; pushExecuted).
std::unique_ptr<Command> setComponent(Scene& scene, Entity e, const InspectorEntry& entry,
                                      std::shared_ptr<void> before, std::shared_ptr<void> after);

} // namespace Commands
} // namespace ember
