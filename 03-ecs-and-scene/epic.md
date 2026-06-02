# Epic 03 — ECS & Scene

**Goal:** Build the data model of the engine. Entities, components, systems, a scene container, parent-child hierarchy, and the event bus. This is the spine that all gameplay and editor features attach to.

**Depends on:** 02-foundation  
**Blocks:** 04-renderer-2d, 06-scripting, 07-editor-v1

---

## Scope

### Event Bus (`engine/events/`)
- `EventBus` — buffered, deferred event queue, processed once per frame
  - `post<T>(args...)` — enqueue event
  - `subscribe<T>(handler)` — returns RAII `SubscriptionToken`
  - `dispatch()` — drain queue, invoke subscribers
- Per-type ring buffers (default capacity 256, configurable per event type)
- Built-in engine event types:
  - `WindowResizeEvent`, `WindowCloseEvent`
  - `KeyPressedEvent`, `KeyReleasedEvent`, `MouseMovedEvent`, `MouseButtonEvent`, `MouseScrollEvent`
  - `SceneLoadedEvent`, `SceneUnloadedEvent`
- `Signal<T>` — immediate synchronous single-event callback (thin EnTT signal wrapper)

### ECS — World & Registry (`engine/ecs/`)
- `Entity` — `uint64_t` (EnTT entity type aliased)
- `World` — owns the EnTT registry, the system scheduler, component lifecycle hooks
  - `create()` → Entity
  - `destroy(Entity)`
  - `emplace<T>(Entity, args...)` → T&
  - `remove<T>(Entity)`
  - `get<T>(Entity)` → T&
  - `tryGet<T>(Entity)` → T*
  - `has<T>(Entity)` → bool
  - `view<T...>()` → iterable range
  - `on_construct<T>()`, `on_destroy<T>()` → Signal&
- `SystemScheduler` — ordered system execution
  - `addSystem<T>(phase)` — phases: PreUpdate, Update, PostUpdate, Render, Debug
  - `runPhase(Phase, f32 dt)`
- `ComponentRegistry` — static registration of component metadata (name, type_index, serialize fn, drawUI fn)
  - `EMBER_REFLECT(T) { EMBER_FIELD(x) ... }` macros

### Built-in Components
```
Transform       — position (vec3), rotation (f32 for 2D / quat for 3D), scale (vec3)
WorldTransform  — cached world-space matrix (dirty-flag updated)
Tag             — string name, string layer
Parent          — Entity parent
Children        — std::vector<Entity> children
```

### Scene (`engine/scene/`)
- `Scene` — named container owning a `World`
  - `create()`, `destroy(Entity)`
  - `instantiate(prefab, position)` → Entity
  - `findByName(string)` → Entity
  - `findByTag(string)` → std::vector<Entity>
  - `setParent(child, parent)` / `getParent(entity)`
  - `getWorldTransform(entity)` — propagates dirty chain
- `SceneManager`
  - `load(path)` — load from `.escene` YAML
  - `loadAdditive(path)`
  - `unload(name)`
  - `getActive()` → Scene&
  - `transition(path)` — load next scene at end of frame
- `TransformSystem` — propagates `WorldTransform` dirty flags each PreUpdate

### Serialization (`engine/serialization/`)
- `EMBER_REFLECT` / `EMBER_FIELD` macro system for compile-time field registration
- `YAMLSerializer`
  - `serialize(Scene&)` → YAML string
  - `deserialize(string, Scene&)` — recreates entities + components
- `YAMLDeserializer` — mirrors serializer
- Scene file format as specified in vision §12
- Prefab support: a single-entity sub-tree saved as a `.eprefab` YAML file

---

## Exit Criteria

- [ ] Can create 1000 entities with Transform + Tag, query and iterate at >60fps with no frame spikes
- [ ] Scene with 3-level entity hierarchy serializes to YAML and round-trips cleanly
- [ ] Event bus posts and dispatches WindowResize; subscriber receives correct data
- [ ] `SceneManager::load` + `save` tested with a multi-entity scene
- [ ] All Catch2 tests pass (ECS CRUD, hierarchy, serialization round-trip)

---

## Key Files Created

```
engine/events/include/ember/events/EventBus.h
engine/events/include/ember/events/Events.h
engine/ecs/include/ember/ecs/World.h
engine/ecs/include/ember/ecs/Entity.h
engine/ecs/include/ember/ecs/SystemScheduler.h
engine/ecs/include/ember/ecs/ComponentRegistry.h
engine/ecs/include/ember/ecs/Components.h       (Transform, Tag, Parent, Children)
engine/scene/include/ember/scene/Scene.h
engine/scene/include/ember/scene/SceneManager.h
engine/scene/systems/TransformSystem.h
engine/serialization/include/ember/serialization/YAMLSerializer.h
engine/serialization/include/ember/serialization/Reflect.h
tests/ecs/TestWorld.cpp
tests/scene/TestSceneSerialization.cpp
```

---

## Notes & Decisions

- EnTT's `registry` is wrapped, not exposed directly — callers use `World`
- `WorldTransform` is a separate component from `Transform` to allow read-only access to the composed matrix without recomputation
- Serialization uses a two-pass approach: first write all entities with their IDs, then write component data — this makes entity cross-references (parent IDs) unambiguous
- Do not add rendering, input, or scripting in this epic — the sandbox test is a headless loop that creates/destroys entities and prints stats
