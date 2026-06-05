#pragma once
#include "CommandHistory.h"
#include "InspectorRegistry.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/Components.h"   // Transform

#include <memory>
#include <string>
#include <vector>

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

// One entity's Transform captured for a gizmo/batch transform edit.
struct EntityXform { Entity e; Transform transform; };

// Restores per-entity Transforms (one undo for a whole gizmo drag / batch move).
std::unique_ptr<Command> transformEntities(Scene& scene,
                                           std::vector<EntityXform> before,
                                           std::vector<EntityXform> after);

// Multi-select component edit: restores per-entity before/after snapshots of one
// component type (single undo). Whole-component sync across the selection.
std::unique_ptr<Command> setComponentBatch(Scene& scene, const InspectorEntry& entry,
                                           std::vector<Entity> entities,
                                           std::vector<std::shared_ptr<void>> before,
                                           std::vector<std::shared_ptr<void>> after);

// Groups several commands into one undo step (executed in order, undone in reverse).
std::unique_ptr<Command> composite(std::string name, std::vector<std::unique_ptr<Command>> cmds);

// One tile change within a paint stroke.
struct TileEdit { u32 x; u32 y; u32 oldId; u32 newId; };

// A tilemap paint stroke (one undo step): redo writes newId, undo restores oldId.
std::unique_ptr<Command> paintTiles(Scene& scene, Entity tilemapEntity, std::vector<TileEdit> edits);

} // namespace Commands
} // namespace ember
