# Epic 06 — Scripting: Requirements

**Status:** Draft  
**Epic:** 06-scripting  
**Depends on:** 03-ecs-and-scene, 05-input-and-assets, 01-vision.md §13  
**Blocks:** 07-editor-v1 (Inspector must display script components)

---

## 1. Overview

Deliver a Unity-`MonoBehaviour`-style **C++** scripting layer: game logic lives in classes that inherit `ScriptComponent`, are attached to entities as ordinary ECS components, and receive lifecycle callbacks (`onStart`/`onUpdate`/…) driven by a `ScriptSystem`. Scripts are the primary game-code API and reuse the engine's existing public surfaces (ECS `World`, `Input`, `AssetManager`, the reflection/serialization system from Epics 03/05). No new third-party dependency.

Scripts are **statically linked** into the game binary this epic; dynamic-library hot reload is Epic 11.

### 1.1 Grounding in the current codebase

| Existing facility (epic) | How scripting uses it |
|--------------------------|-----------------------|
| `ISystem`/`SystemScheduler`, `Phase` = {PreUpdate, Update, PostUpdate, Render, Debug} (03) | `ScriptSystem` is one `ISystem` at `Phase::Update`; **no** `LateUpdate`/`FixedUpdate` phases exist, so it runs those passes itself |
| `World` (entt-backed): `create/destroy/emplace<T>/get<T>/tryGet<T>/view<...>()` (03) | each script type is a normal entt component; `view<T>()` enumerates instances |
| `ComponentRegistry` + `ComponentMeta{serialize,deserialize,drawUI}` and `ember::registerComponentType<T>(name)` (03/05 MIG-04) | scripts register the same way → free serialization + future inspector |
| `EMBER_REFLECT(T){ EMBER_FIELD(x); }` free-function reflection (03) | reused verbatim for script fields |
| `Input::isKeyDown/isKeyPressed`, `Input::getAction(name) → ActionState{pressed,held,released,axisValue}` (05) | sample scripts read input through this facade |
| `AssetHandle<T>`, `AssetManager`, `Time::update/delta` (02/05) | serialized script fields, prefab spawning, dt |

### 1.2 Key design decision — how polymorphic scripts live in the ECS

`ScriptComponent` is polymorphic, but entt stores components by concrete type. We reconcile this so that `world.emplace<PlayerController>(e)` (as the vision shows) still works:

- **Each script subclass is its own entt component**, stored by value. The base pointer is `static_cast<ScriptComponent*>(&world.get<T>(e))`.
- `EMBER_REGISTER_SCRIPT(T)` registers, per type, an **enumerator** `void(World&, fn(Entity, ScriptComponent&))` implemented as `for (auto [e, c] : world.view<T>()) fn(e, c);`. `ScriptSystem` calls every registered enumerator to build one flat, order-sorted `std::vector<ScriptEntry>` and iterates that directly — **no virtual dispatch to find scripts**, only to invoke their overrides (per §13 "flat vector of {entity, script*}").

This keeps scripts serializable as normal components and avoids a `std::unique_ptr` indirection per script.

---

## 2. Definitions

- **Script / script component** — an instance of a class deriving `ScriptComponent`, attached to one entity.
- **Active** — a script whose entity exists and whose `onStart` has fired.
- **Script registry** — the per-type table (factory, enumerator, execution order, reflected meta) populated by `EMBER_REGISTER_SCRIPT`.
- **Deferred** — performed at end-of-frame rather than immediately (entity destruction, `onStart` for newly-added scripts).

---

## 3. ScriptComponent base class (`engine/scripting/ScriptComponent.h`)

- **SC-01** Abstract base `ScriptComponent` with virtual destructor and virtual lifecycle hooks, all defaulting to no-ops: `onStart()`, `onUpdate(f32 dt)`, `onFixedUpdate(f32 dt)`, `onLateUpdate(f32 dt)`, `onDestroy()`.
- **SC-02** Declared-but-not-yet-invoked physics callbacks (wired by a future physics epic; present so script code compiles against them): `onCollisionEnter/Stay/Exit(Entity)`, `onTriggerEnter/Exit(Entity)`.
- **SC-03** Protected component helpers operating on the owning entity: `getComponent<T>()` (asserts present), `tryGetComponent<T>()`, `addComponent<T>()`, `removeComponent<T>()`, `hasComponent<T>()`.
- **SC-04** Protected entity/scene access: `getEntity()`, `getScene()`, `getWorld()`, `destroy()` (deferred), `instantiate(AssetHandle<Prefab>)` (see SC-08).
- **SC-05** Convenience: `getName()/setName()` (via the entity's `Tag`), `transform()` (shorthand for `getComponent<Transform>()`).
- **SC-06** The base stores `Entity m_entity`, `Scene* m_scene`, and lifecycle bookkeeping (`bool m_started`, `bool m_enabled`), all `private`/`protected` and populated by `ScriptSystem` (declared `friend`). Game code never sets them.
- **SC-07** Helpers must be safe to call from any lifecycle hook (including `onStart`); they resolve through `m_scene`/`m_entity` set before `onStart`.
- **SC-08** `instantiate(AssetHandle<Prefab>)` returns a new `Entity`. **Dependency:** a `Prefab` asset type + loader does not yet exist (Epic 05 left prefab emit/load helpers in serialization but no `Prefab` asset). This epic MAY ship `instantiate` against an in-memory prefab/scene source and treat the `AssetHandle<Prefab>` overload as a thin wrapper once the Prefab asset lands; if not, `instantiate` is declared and stubbed with a logged warning. (Tracked as an open item, §11.)

---

## 4. ScriptSystem (`engine/scripting/ScriptSystem.h/.cpp`)

- **SS-01** `ScriptSystem : ISystem`, added at `Phase::Update`. Constructed with a `Scene&`/`World&` (and access to the registry) so it can inject `m_scene`/`m_entity` and resolve helpers.
- **SS-02** Each `update(world, dt)` it (re)builds the flat `std::vector<ScriptEntry{Entity, ScriptComponent*, order}>` from all registered enumerators, sorted per §6. Rebuild cost is acceptable for this epic (NFR-02); incremental maintenance via entt signals is a permitted optimization.
- **SS-03** **onStart** fires exactly once per script, on the first `ScriptSystem` tick after the script is observed (i.e. the frame *after* `emplace`, never before), before that script's first `onUpdate`. Tracked by `m_started`.
- **SS-04** **onUpdate(dt)** fires every frame for every active, enabled script, in execution order.
- **SS-05** **onLateUpdate(dt)** fires for all scripts *after* every `onUpdate` has run that frame (enables camera-follow).
- **SS-06** **onFixedUpdate(dt)** driven by a fixed-timestep accumulator; default **50 Hz** (`fixedDelta = 0.02s`), configurable; may run zero or several times per frame; uses the fixed delta, not the frame dt. No interpolation this epic.
- **SS-07** **onDestroy** fires before the entity leaves the world, for scripts whose entity was destroyed (via `destroy()` or `World::destroy`). Destruction triggered through a script's `destroy()` is **deferred to end-of-frame**; `onDestroy` runs then, before the actual `world.destroy(e)`.
- **SS-08** **Exception safety:** every script callback invocation is wrapped; a thrown `std::exception` (or `...`) is caught, logged via `EMBER_LOG_ERROR` (script type + entity + what()), and that script is **disabled** (no further callbacks) so one bad script never crashes or stalls the engine (NFR-03).
- **SS-09** Disabled/destroyed scripts are skipped on subsequent ticks; the flat list tolerates entities/components removed mid-iteration (resolve pointers freshly or guard with `tryGet`).
- **SS-10** Ordering of the four per-frame passes within one `update`: onStart (new) → onUpdate → onLateUpdate, with onFixedUpdate executed before onUpdate for each due fixed step (documented and deterministic).

---

## 5. Script registration (`engine/scripting/ScriptRegistry.h`)

- **REG-01** `EMBER_REGISTER_SCRIPT(Type)` — a single macro placed in a game `.cpp` that emits a static initializer registering `Type`. It must:
  - register `Type` with `ComponentRegistry` for serialization (reusing `registerComponentType<Type>("Type")` semantics from MIG-04), and
  - register with the `ScriptRegistry`: a **factory** (`[]{ return std::unique_ptr<ScriptComponent>... }` or an entt-`emplace` adder), an **enumerator** over `World::view<Type>()`, the **type name**, and the **execution order** (default 0).
- **REG-02** Registration is idempotent and order-independent across translation units (guard on type/name as `ComponentRegistry`/`registerComponentType` already do).
- **REG-03** The `ScriptRegistry` is a process-global, query-able table: list registered scripts, look up by name, get factory/enumerator/order. (Mirrors `ComponentRegistry::instance()`.)
- **REG-04** A script must be *registerable without serialization* too: a non-reflected script (no `EMBER_REFLECT`) still runs; it simply has no serialized fields and an empty inspector (consistent with the optional-`EMBER_FIELD` note in the epic). The macro must handle "reflected" vs "not reflected" gracefully (see SER-03).

---

## 6. Execution order (`ScriptRegistry`)

- **ORD-01** Default execution order is the integer **priority 0**; lower priority runs earlier.
- **ORD-02** `EMBER_SCRIPT_ORDER(Type, n)` sets an explicit priority for `Type`.
- **ORD-03** Ties (equal priority) break **alphabetically by type name**, so ordering is fully deterministic and stable across runs/platforms.
- **ORD-04** The flat list is sorted by `(priority, typeName)`; within a single type, entity iteration order follows the entt view (stable for the run).

---

## 7. Script serialization (reuses Epics 03/05)

- **SER-01** Script fields decorated with `EMBER_FIELD` (inside an `EMBER_REFLECT(Type){…}` block, consistent with components) serialize to / deserialize from YAML automatically via the existing reflection visitors.
- **SER-02** Natively supported field types: `f32`, `i32`, `u32`, `u64`, `bool`, `std::string`, `glm::vec2/3/4`, `Entity` — exactly what `YAMLWriteVisitor`/`YAMLReadVisitor` already handle. **`AssetHandle<T>`** support is **new this epic**: add visitor overloads that serialize a handle by its asset's virtual path / UUID (resolved via `AssetDatabase`) and re-resolve on load (handles are runtime ids and must not be written raw). If a script declares an `AssetHandle<T>` field, this overload is required (SER-04).
- **SER-03** Unknown / unsupported field types are **skipped with a warning**, never a hard error (the visitor simply has no overload → must degrade gracefully; achieved by the macro only reflecting supported fields, or a fallback overload that logs once).
- **SER-04** Round-trip fidelity: a script's reflected scalar fields restore to equal values; `AssetHandle<T>` fields restore to a handle referring to the same asset path.
- **SER-05** Non-reflected (runtime-only) members — e.g. cached pointers like `RigidBody* rb` — are never serialized.

---

## 8. Sandbox sample scripts (`sandbox/scripts/`)

Validate the system end-to-end:

- **SBX-01** `PlayerController` — WASD / arrow movement and Space-to-jump, moving a sprite's `Transform` directly (no physics yet); reads input via `Input::getAction("MoveX/MoveY").axisValue` and `Input::isKeyPressed(Key::Space)`; serialized `speed`, `jumpForce`.
- **SBX-02** `CameraFollow` — in `onLateUpdate`, lerps a `Camera2D`/`EditorCamera` toward a target entity's position; serialized `target` (Entity) and `smoothing`.
- **SBX-03** `Spinner` — rotates its `Transform` by `speed * dt` each `onUpdate`; serialized `speed`.
- **SBX-04** `Lifetime` — destroys its entity after `duration` seconds (`destroy()`), proving deferred destroy + `onDestroy`.
- **SBX-05** The sandbox registers these via `EMBER_REGISTER_SCRIPT`, adds a `ScriptSystem` at `Phase::Update`, and drives it in the existing loop alongside `Renderer2D`/`SpriteRenderSystem`.

> Sample scripts that need input/asset facilities reuse the Epic 05 systems already wired in the sandbox. Camera-follow targets the existing `EditorCamera`/`Camera2D`.

---

## 9. Test requirements (`tests/scripting/`)

CPU-only, headless (no GL); driven by ticking a `ScriptSystem` over a `World`.

- **TST-01** `onStart` fires exactly once and only after `emplace` (not on the emplace frame) — assert via a counting test script.
- **TST-02** `onUpdate` fires every tick; `onLateUpdate` fires after all `onUpdate`s (assert ordering with a shared log).
- **TST-03** `onFixedUpdate` accumulator: N ticks of total elapsed `t` produce `floor(t / fixedDelta)` fixed calls, each with `fixedDelta`.
- **TST-04** `onDestroy` fires before the entity is removed; deferred `destroy()` removes the entity by end-of-frame (`Lifetime`-style).
- **TST-05** Execution order respects priority then type name (two scripts with set/observed order).
- **TST-06** Exception in `onUpdate` is caught, logged, and disables only that script; other scripts keep running and the engine doesn't crash.
- **TST-07** Script field serialization round-trips through YAML (scalar fields; and an `AssetHandle<T>` field via path if SER-02 lands).
- **TST-08** Perf smoke (may be a `[.benchmark]`/optional case): 500 `Spinner`s tick without error (the >60 fps target is validated in the sandbox, not unit tests).

---

## 10. Non-Functional Requirements

- **NFR-01** New module `engine/scripting/` → `ember_scripting` static lib. Depends on `ember_core`, `ember_ecs`, `ember_scene`, and (for `AssetHandle` fields / `instantiate`) `ember_assets`; `ember_serialization` + yaml-cpp are **PRIVATE** (only the registration/serialization TU sees yaml, mirroring the renderer's MIG-04 pattern). No GLFW/GL.
- **NFR-02** Per-frame script dispatch is allocation-light in steady state (rebuild may allocate; prefer reuse of the flat vector). The inner loop avoids per-script heap allocation and per-script map lookups.
- **NFR-03** A misbehaving script (exception, missing component accessed via `tryGet`) must never crash the engine (SS-08).
- **NFR-04** Public scripting headers stay free of yaml-cpp/entt-leaking includes where practical (entt types may appear via `World`/`view` in the inline helpers — acceptable, consistent with existing `World.h`).
- **NFR-05** Deterministic ordering (ORD-03) so behavior is reproducible across runs and platforms.
- **NFR-06** Builds clean under the project's `-Wall -Wextra -Wpedantic -Werror` on GCC/Clang/MSVC; tests pass under the `sanitize` preset (no leaks/UB in lifecycle/destroy paths).

---

## 11. Out of scope / open items

- **Dynamic hot reload** (shared-library swap of scripts) — Epic 11 / vision Milestone 9.
- **Lua or other embedded VM** — not planned; scripting is C++.
- **Physics-driven callbacks** — `onCollision*`/`onTrigger*` are declared (SC-02) but not invoked until the physics epic.
- **`Prefab` asset type + loader** — needed for the `AssetHandle<Prefab>` form of `instantiate` (SC-08); if it doesn't land here, `instantiate` ships stubbed/logged and is completed alongside the Prefab asset.
- **Inspector UI** (`drawUI`) — Epic 07; this epic only ensures scripts register so the inspector *can* find them.
- **`Input::getAxis(name)` convenience** — vision shows `Input::getAxis("Horizontal")`; Epic 05 shipped `getAction(name).axisValue`. An optional thin `getAxis` wrapper may be added; otherwise scripts use `getAction`.
- **Coroutines / timers / message passing between scripts** — future.

---

## 12. Acceptance Criteria Summary

- [ ] `PlayerController` on a sprite moves it with WASD at the configured speed; Space triggers the jump path.
- [ ] `CameraFollow` smoothly tracks the player across screen boundaries (`onLateUpdate`).
- [ ] `onStart` fires exactly once, the frame after `emplace`, before first `onUpdate`.
- [ ] `onDestroy` fires before the entity is removed; deferred `destroy()` removes it by end-of-frame.
- [ ] Script scalar fields (`PlayerController` speed/jumpForce) serialize to YAML and round-trip.
- [ ] An exception thrown in `onUpdate` is logged and disables only that script — no crash.
- [ ] 500 `Spinner` entities rotate independently at >60 fps (sandbox).
- [ ] `ember_scripting` builds clean (`-Werror`); `tests/scripting/*` pass; no yaml/GL in public scripting headers.
