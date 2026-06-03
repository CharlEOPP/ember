# Epic 03 — ECS & Scene: Requirements

**Status:** Draft  
**Epic:** 03-ecs-and-scene  
**Depends on:** 02-foundation, 01-vision.md  
**Blocks:** 04-renderer-2d, 06-scripting, 07-editor-v1

---

## 1. Overview

This document specifies the complete requirements for the ECS & Scene epic — the data model and structural spine of the engine. It delivers:

1. An **Event System**: a deferred, buffered `EventBus` plus an immediate `Signal<T>` mechanism.
2. An **ECS layer**: a `World` wrapping the EnTT registry, with create/destroy/component CRUD, queries, and lifecycle hooks.
3. A **System Scheduler**: ordered, phased system execution.
4. **Built-in components**: `Transform`, `WorldTransform`, `Tag`, `Parent`, `Children`.
5. A **reflection system** (`EMBER_REFLECT` / `EMBER_FIELD`) feeding a `ComponentRegistry`.
6. A **Scene** container with parent-child hierarchy and cached world transforms, plus a `SceneManager`.
7. **YAML serialization** that round-trips a multi-entity, multi-level scene cleanly.

Nothing rendered, scripted, or input-driven is built here. The measure of success is a headless sandbox that creates, queries, and destroys thousands of entities, plus a scene that serializes and deserializes with full fidelity — all verified by tests and green CI.

---

## 2. Definitions

| Term | Meaning |
|------|---------|
| **ECS** | Entity-Component-System — the data-oriented object model |
| **Entity** | A 64-bit opaque identifier (index + generation) for a game object |
| **Component** | A plain data struct with no behaviour, attached to an entity |
| **System** | A function/class that queries and transforms components each phase |
| **World** | Owner of the EnTT registry, scheduler, and lifecycle hooks |
| **EventBus** | Deferred, per-frame buffered event queue |
| **Signal\<T\>** | Immediate, synchronous single-event callback (EnTT signal wrapper) |
| **Reflection** | Compile-time enumeration of a component's fields via `EMBER_REFLECT` |
| **Scene** | A named container owning a `World` and its hierarchy/settings |
| **Prefab** | A single-entity sub-tree saved to a `.eprefab` YAML file |
| **Dirty flag** | A marker that a `WorldTransform` must be recomputed |
| **Phase** | A named slot in the system execution order (PreUpdate, Update, …) |
| **`.escene`** | A scene saved as YAML |
| **`.eprefab`** | A prefab saved as YAML |

---

## 3. Event System Requirements (`engine/events/`)

### 3.1 EventBus — Deferred Queue

| ID | Requirement |
|----|-------------|
| EVT-01 | `EventBus::post<T>(args...)` SHALL construct an event of type `T` in-place and enqueue it for later dispatch. It SHALL NOT invoke subscribers immediately. |
| EVT-02 | `EventBus::subscribe<T>(handler)` SHALL register a callable `void(const T&)` and SHALL return an RAII `SubscriptionToken`. |
| EVT-03 | When a `SubscriptionToken` is destroyed, its handler SHALL be automatically unregistered; no further events SHALL be delivered to it. |
| EVT-04 | A moved-from `SubscriptionToken` SHALL NOT unregister on destruction (move-only semantics). Tokens SHALL NOT be copyable. |
| EVT-05 | `EventBus::dispatch()` SHALL drain all queued events, invoking every current subscriber for each event's type, then clear the queue. It is intended to be called once per frame by the main loop. |
| EVT-06 | Events of a given type SHALL be dispatched in FIFO (post) order. |
| EVT-07 | Each event type SHALL be backed by a ring buffer with a default capacity of 256 events. |
| EVT-08 | The per-type ring buffer capacity SHALL be configurable per event type (e.g., `EventBus::reserve<T>(capacity)`). |
| EVT-09 | If a ring buffer overflows within a single frame, the bus SHALL log a warning via `EMBER_LOG_WARN` and SHALL NOT crash or silently drop without notice. |
| EVT-10 | Subscriber lists SHALL be keyed by `std::type_index`; posting or subscribing to an event type with no subscribers SHALL be a safe no-op. |
| EVT-11 | The `EventBus` is single-threaded by contract: `post`, `subscribe`, and `dispatch` SHALL be called from the main thread only. This constraint SHALL be documented in the header. |

### 3.2 Built-in Event Types (`Events.h`)

| ID | Requirement |
|----|-------------|
| EVT-12 | The following window event types SHALL be defined: `WindowResizeEvent { u32 width, height; }`, `WindowCloseEvent {}`. (These supersede the stub types from Epic 02.) |
| EVT-13 | The following input event types SHALL be defined as plain structs (produced by the Input epic, not this one): `KeyPressedEvent`, `KeyReleasedEvent`, `MouseMovedEvent`, `MouseButtonEvent`, `MouseScrollEvent`. |
| EVT-14 | The following scene event types SHALL be defined: `SceneLoadedEvent { std::string name; }`, `SceneUnloadedEvent { std::string name; }`. |
| EVT-15 | All event types SHALL be plain aggregates (no virtual methods); the bus SHALL NOT require a common base class. |

### 3.3 Signal — Immediate Dispatch

| ID | Requirement |
|----|-------------|
| EVT-16 | `Signal<T>` SHALL wrap EnTT's signal/sink mechanism to provide synchronous, immediate callbacks. |
| EVT-17 | `Signal<T>::connect(handler)` SHALL register a callback invoked immediately when the signal is fired; `fire(const T&)` SHALL invoke all connected handlers in connection order. |
| EVT-18 | `Signal<T>` SHALL be used for component lifecycle hooks (see ECS-13) and tight system coupling; `EventBus` SHALL be used for decoupled, frame-deferred communication. The distinction SHALL be documented. |

---

## 4. ECS Requirements (`engine/ecs/`)

### 4.1 Entity (`Entity.h`)

| ID | Requirement |
|----|-------------|
| ECS-01 | `Entity` SHALL be a strong type alias of the EnTT entity handle, configured as a 64-bit identifier (index + generation). |
| ECS-02 | A constant `NULL_ENTITY` SHALL represent the invalid/null entity. |
| ECS-03 | `Entity` SHALL be equality-comparable and usable as a key in `std::unordered_map`/`std::unordered_set`. |

### 4.2 World (`World.h`)

| ID | Requirement |
|----|-------------|
| ECS-04 | `World` SHALL own a single EnTT `registry`. The EnTT registry SHALL NOT be exposed in the public API; all access SHALL go through `World`. |
| ECS-05 | `World::create()` SHALL return a new, unique `Entity`. |
| ECS-06 | `World::destroy(Entity)` SHALL remove the entity and all its components. Destroying an already-destroyed or `NULL_ENTITY` SHALL trigger `EMBER_ASSERT` (debug) and be a safe no-op otherwise. |
| ECS-07 | `World::emplace<T>(Entity, args...)` SHALL construct component `T` on the entity and return `T&`. Emplacing a `T` that already exists SHALL trigger `EMBER_ASSERT`. |
| ECS-08 | `World::remove<T>(Entity)` SHALL remove component `T` from the entity. Removing an absent `T` SHALL be a safe no-op. |
| ECS-09 | `World::get<T>(Entity)` SHALL return `T&`. Calling `get` for an absent component SHALL trigger `EMBER_ASSERT`. |
| ECS-10 | `World::tryGet<T>(Entity)` SHALL return `T*`, or `nullptr` if the component is absent. |
| ECS-11 | `World::has<T>(Entity)` SHALL return `bool`. `has<T...>` SHALL return true only if all listed components are present. |
| ECS-12 | `World::view<T...>()` SHALL return an iterable range yielding `(Entity, T&...)` via structured bindings. |
| ECS-13 | `World::on_construct<T>()` and `World::on_destroy<T>()` SHALL each return a `Signal&` (or EnTT sink) that fires when a component of type `T` is added/removed. |
| ECS-14 | `World::valid(Entity)` SHALL return whether the entity currently exists. |
| ECS-15 | `World::clear()` SHALL destroy all entities and components, leaving an empty world. |

### 4.3 System Scheduler (`SystemScheduler.h`)

| ID | Requirement |
|----|-------------|
| ECS-16 | A `Phase` enum SHALL define, in execution order: `PreUpdate`, `Update`, `PostUpdate`, `Render`, `Debug`. |
| ECS-17 | `SystemScheduler::addSystem<T>(phase, args...)` SHALL register a system in the given phase. |
| ECS-18 | `SystemScheduler::runPhase(Phase, f32 dt)` SHALL execute every system registered to that phase, in registration order, passing the `World` and `dt`. |
| ECS-19 | A system SHALL be any type exposing `void update(World&, f32 dt)` (or an equivalent callable contract). |
| ECS-20 | Parallel execution of independent systems is OUT OF SCOPE for this epic; execution SHALL be deterministic and single-threaded. (The API SHALL not preclude adding parallelism later.) |

### 4.4 Built-in Components (`Components.h`)

| ID | Requirement |
|----|-------------|
| CMP-01 | All built-in components SHALL be plain data structs containing no behaviour (no virtual methods, no invariants enforced in methods). |
| CMP-02 | `Transform` SHALL contain: `glm::vec3 position`, `f32 rotation` (radians, 2D), `glm::vec3 scale` (default `{1,1,1}`). |
| CMP-03 | `WorldTransform` SHALL contain a cached `glm::mat4 matrix` and a `bool dirty` flag. It SHALL be a separate component from `Transform` so consumers can read the composed matrix without recomputation. |
| CMP-04 | `Tag` SHALL contain: `std::string name`, `std::string layer`. |
| CMP-05 | `Parent` SHALL contain a single `Entity parent`. |
| CMP-06 | `Children` SHALL contain a `std::vector<Entity> children`. |
| CMP-07 | Every built-in component SHALL be registered with the `ComponentRegistry` via `EMBER_REFLECT` so it serializes without bespoke code. |

### 4.5 Component Registry & Reflection (`ComponentRegistry.h`, `Reflect.h`)

| ID | Requirement |
|----|-------------|
| REF-01 | `EMBER_REFLECT(T) { EMBER_FIELD(x) ... }` SHALL register component `T` and enumerate its serializable fields at static-init time. |
| REF-02 | `EMBER_FIELD(member)` SHALL capture the field's name (as a string) and a means to read/write it for serialization. |
| REF-03 | The `ComponentRegistry` SHALL store, per registered component: the type name (string), `std::type_index`, a serialize function, and a deserialize function. |
| REF-04 | The registry SHALL reserve a metadata slot for a `drawUI` function for the future inspector; in this epic that slot MAY be null and SHALL NOT be invoked. |
| REF-05 | The registry SHALL support lookup by type name and by `std::type_index`. |
| REF-06 | Registering the same component type twice SHALL trigger `EMBER_ASSERT` (debug) and SHALL NOT create a duplicate entry. |
| REF-07 | Supported field types for serialization in this epic SHALL include at minimum: the `Types.h` scalar aliases, `bool`, `std::string`, `glm::vec2/3/4`, `Entity`, and `std::vector<Entity>`. |

---

## 5. Scene Requirements (`engine/scene/`)

### 5.1 Scene (`Scene.h`)

| ID | Requirement |
|----|-------------|
| SCN-01 | `Scene` SHALL own exactly one `World` and SHALL have a `std::string name`. |
| SCN-02 | `Scene` SHALL hold a `SceneSettings` struct including at minimum `glm::vec4 backgroundColor`. |
| SCN-03 | `Scene::create()` SHALL create an entity in the scene's world (convenience wrapper). `Scene::destroy(Entity)` SHALL destroy it and detach it from any parent/children. |
| SCN-04 | `Scene::findByName(std::string_view)` SHALL return the first entity whose `Tag.name` matches, or `NULL_ENTITY`. |
| SCN-05 | `Scene::findByTag(std::string_view layer)` SHALL return a `std::vector<Entity>` of all entities whose `Tag.layer` matches. |
| SCN-06 | `Scene::instantiate(prefab, glm::vec3 position)` SHALL create the prefab's entity sub-tree in the scene and set the root's `Transform.position`. (Prefab is loaded from a `.eprefab` file; asset-handle integration is deferred to a later epic.) |
| SCN-07 | `Scene::setParent(child, parent)` SHALL establish the parent-child relationship by maintaining the `Parent` and `Children` components on both entities. |
| SCN-08 | `Scene::getParent(Entity)` SHALL return the parent entity or `NULL_ENTITY` if the entity is a root. |
| SCN-09 | Re-parenting an entity SHALL remove it from its previous parent's `Children` before adding it to the new parent's. A cycle (making an entity its own ancestor) SHALL trigger `EMBER_ASSERT` and SHALL be rejected. |
| SCN-10 | `Scene::getWorldTransform(Entity)` SHALL return the composed world-space `glm::mat4`, recomputing along the dirty chain from the nearest clean ancestor as needed. |

### 5.2 Transform Propagation

| ID | Requirement |
|----|-------------|
| SCN-11 | Mutating an entity's `Transform` SHALL mark its `WorldTransform` (and, transitively, its descendants') `dirty` flag. |
| SCN-12 | A `TransformSystem` SHALL run in the `PreUpdate` phase and SHALL recompute `WorldTransform.matrix` for all dirty entities, propagating from parents to children so a parent is resolved before its children. |
| SCN-13 | A root entity's world transform SHALL equal its local transform. A child's world transform SHALL equal `parentWorld * childLocal`. |

### 5.3 SceneManager (`SceneManager.h`)

| ID | Requirement |
|----|-------------|
| SCM-01 | `SceneManager::load(path)` SHALL load a `.escene` file, replacing the active scene, and SHALL post a `SceneLoadedEvent`. |
| SCM-02 | `SceneManager::loadAdditive(path)` SHALL load a scene on top of the current active set without unloading existing scenes. |
| SCM-03 | `SceneManager::unload(name)` SHALL unload the named scene and SHALL post a `SceneUnloadedEvent`. |
| SCM-04 | `SceneManager::getActive()` SHALL return a reference to the active `Scene`. Behaviour when no scene is active SHALL be defined (assert or documented sentinel). |
| SCM-05 | `SceneManager::transition(path)` SHALL defer loading the next scene until the end of the current frame (no mid-frame entity invalidation). |
| SCM-06 | Loading a scene file that does not exist or fails to parse SHALL return an error (via `Result`) and SHALL NOT corrupt the currently active scene. |

---

## 6. Serialization Requirements (`engine/serialization/`)

| ID | Requirement |
|----|-------------|
| SER-01 | `YAMLSerializer::serialize(const Scene&)` SHALL return a YAML `std::string` representing the scene. |
| SER-02 | `YAMLDeserializer::deserialize(const std::string&, Scene&)` SHALL recreate entities and components from YAML into the given scene, returning a `Result`. |
| SER-03 | Serialization SHALL be reflection-driven: components SHALL be written/read using the `ComponentRegistry` metadata, with no per-component hand-written serialization code. |
| SER-04 | Deserialization SHALL use a two-pass approach: first create all entities with their stored IDs, then populate component data — so cross-entity references (e.g., `Parent`/`Children`) resolve unambiguously. |
| SER-05 | The scene file SHALL match the format in vision §12: a top-level `version`, `name`, `settings`, and an `entities` list, where each entity has `id`, `name`, a `components` map, and an optional nested `children` list. |
| SER-06 | The serializer SHALL write a `version` field; the deserializer SHALL reject a `version` it does not understand with a clear error. |
| SER-07 | A component present in the file but not registered SHALL be skipped with an `EMBER_LOG_WARN`; it SHALL NOT abort the load. |
| SER-08 | A round-trip (serialize → deserialize → serialize) of any scene SHALL produce byte-identical YAML on the second serialization (stable field ordering). |
| SER-09 | Entity IDs SHALL be preserved across a save/load round-trip. |
| SER-10 | Prefabs SHALL be supported: a single-entity sub-tree SHALL save to and load from a `.eprefab` YAML file using the same component-serialization machinery. |
| SER-11 | No yaml-cpp type SHALL appear in any public header outside `engine/serialization/`; YAML SHALL be an implementation detail. |

---

## 7. Test Requirements (`tests/`)

### 7.1 Framework & Integration

| ID | Requirement |
|----|-------------|
| TST-01 | New tests SHALL be added to the existing `ember_tests` Catch2 target and discovered via `catch_discover_tests`. |
| TST-02 | All new tests SHALL pass under `ctest --preset debug` and `--preset release` on all three CI platforms. |

### 7.2 Required Test Cases

#### Events
| Test | Verifies | Req |
|------|----------|-----|
| `EventBus_PostDispatchDelivers` | A posted `WindowResizeEvent` is delivered to a subscriber with correct field values after `dispatch()` | EVT-01/05 |
| `EventBus_DeferredNotImmediate` | A subscriber is NOT invoked by `post` alone (only after `dispatch`) | EVT-01 |
| `EventBus_TokenAutoUnsubscribe` | A handler stops receiving events once its `SubscriptionToken` is destroyed | EVT-03 |
| `EventBus_MultipleSubscribers` | All subscribers to a type receive the event, in subscription order | EVT-05/06 |
| `EventBus_RingBufferCapacity` | Posting beyond default capacity logs a warning and does not crash | EVT-07/09 |
| `Signal_ImmediateInvoke` | `Signal<T>::fire` invokes connected handlers synchronously | EVT-16/17 |

#### ECS
| Test | Verifies | Req |
|------|----------|-----|
| `World_CreateDestroy` | `create` yields valid entities; `destroy` invalidates them | ECS-05/06/14 |
| `World_ComponentCRUD` | `emplace`/`get`/`has`/`remove` behave correctly; `tryGet` returns null when absent | ECS-07/08/09/10/11 |
| `World_ViewIteration` | `view<Transform, Tag>()` iterates exactly the matching entities | ECS-12 |
| `World_LifecycleHooks` | `on_construct<T>`/`on_destroy<T>` fire with the right entity | ECS-13 |
| `Scheduler_PhaseOrder` | Systems run in phase order and registration order; `dt` is forwarded | ECS-17/18 |

#### Scene & Hierarchy
| Test | Verifies | Req |
|------|----------|-----|
| `Scene_FindByNameAndTag` | `findByName`/`findByTag` return the correct entities | SCN-04/05 |
| `Scene_SetGetParent` | `setParent`/`getParent` and `Children` stay consistent; re-parent removes from old parent | SCN-07/08/09 |
| `Transform_WorldPropagation` | A 3-level hierarchy resolves world transforms as `parentWorld * childLocal` | SCN-10/12/13 |
| `Transform_DirtyOnMutation` | Mutating a parent `Transform` marks descendants dirty | SCN-11 |

#### Serialization
| Test | Verifies | Req |
|------|----------|-----|
| `Serialize_MultiEntityRoundtrip` | A scene of several entities serializes and deserializes with identical state | SER-01/02 |
| `Serialize_HierarchyRoundtrip` | A 3-level entity hierarchy round-trips cleanly (parent/children intact) | SER-04/09 |
| `Serialize_StableOutput` | Double serialization yields byte-identical YAML | SER-08 |
| `Serialize_UnknownComponentSkipped` | An unregistered component in the file is skipped with a warning, not a crash | SER-07 |
| `Serialize_PrefabRoundtrip` | A `.eprefab` single-entity sub-tree saves and instantiates correctly | SER-10 |

#### Performance (smoke)
| Test | Verifies | Req |
|------|----------|-----|
| `ECS_ThousandEntitiesIterate` | Creating 1000 entities with `Transform + Tag` and iterating a view completes within the NFR-01 budget | NFR-01 |

---

## 8. Non-Functional Requirements

| ID | Requirement |
|----|-------------|
| NFR-01 | **Throughput.** Creating 1000 entities with `Transform + Tag`, then iterating a `view<Transform, Tag>()`, SHALL complete within a single frame budget (≤ 16ms) on a modern developer machine, with no per-iteration heap allocation. |
| NFR-02 | **Deterministic serialization.** Scene serialization SHALL produce stable, ordered output so round-trips are reproducible and diff-friendly. |
| NFR-03 | **No UB / leaks.** The headless ECS/scene sandbox and the test suite SHALL produce zero ASan/UBSan reports under the `sanitize` preset and a clean leak report on shutdown. |
| NFR-04 | **Header hygiene.** No EnTT header SHALL appear in any public `engine/ecs` header, and no yaml-cpp header in any public `engine/serialization` header; both SHALL be implementation details. Including any single public header from this epic SHALL NOT pull in EnTT or yaml-cpp. |
| NFR-05 | **Backend isolation.** Swapping the ECS backend (EnTT → other) SHALL require changes only within `engine/ecs/` implementation files, not in consumer code. |
| NFR-06 | **Warning-free.** All new code SHALL compile with zero warnings under `-Wall -Wextra -Wpedantic -Werror` (GCC/Clang) and `/W4 /WX` (MSVC). |

---

## 9. Out of Scope

The following are explicitly deferred to later epics and SHALL NOT be implemented here:

- Rendering of any kind (sprites, batching, cameras) — Epic 04.
- Producing input events from real devices (the event *types* are defined, but the input system is) — Epic 05.
- Asset-handle–based prefab references (prefabs load from file only this epic) — Epics 05/09.
- Component `drawUI` / inspector rendering (metadata slot reserved, not invoked) — Epic 07.
- Scripting and script components — Epic 06.
- Parallel/multithreaded system execution — future.
- Binary (shipped) serialization format — future; YAML only here.

---

## 10. Acceptance Criteria Summary

The epic is complete when ALL of the following are true:

1. 1000 entities with `Transform + Tag` can be created, queried, and iterated within one frame budget with no frame spikes (NFR-01).
2. A scene with a 3-level entity hierarchy serializes to YAML and round-trips cleanly, preserving IDs and parent/child links.
3. The `EventBus` posts and dispatches a `WindowResizeEvent`; the subscriber receives the correct data, and `SubscriptionToken` auto-unsubscribe works.
4. `SceneManager::load` and serialization save are verified against a multi-entity scene.
5. All Catch2 tests pass (ECS CRUD, hierarchy, transform propagation, serialization round-trip, events) under both debug and release.
6. No EnTT or yaml-cpp type leaks into public headers (NFR-04).
7. Running the headless ECS/scene sandbox under the `sanitize` preset produces no ASan/UBSan errors.
8. GitHub Actions CI is green on `main` for all three platform jobs, with zero warnings.
