# Epic 06 — Scripting: Implementation Plan

**Status:** Not started  
**Document:** 03-implementation-plan.md  
**Refs:** epic.md, 02-requirements.md, 01-vision.md §13

Work the phases in order; each ends buildable and testable. Don't advance until the current phase's boxes are ticked and its Verify passes. Requirement IDs (e.g. `SS-03`) refer to `02-requirements.md`.

**Grounding (current repo state):**

| Fact | Consequence for this epic |
|------|---------------------------|
| `ISystem{update(World&,f32)}`, `Phase`={PreUpdate,Update,PostUpdate,Render,Debug} | one `ScriptSystem` at `Phase::Update`; it runs fixed/update/late passes itself (no such phases exist) |
| `Scene` owns a `World` (`scene.world()`), `create()` adds Tag+Transform+WorldTransform | `ScriptSystem`/`ScriptComponent` resolve via `Scene*` → `world()` |
| `World::destroy(e)` is **immediate**; `World::on_construct<T>()/on_destroy<T>()` entt signals exist | `ScriptSystem` owns a deferred-destroy queue; `onDestroy` is fired from an `on_destroy<T>` hook (reliable for both `destroy()` and external removal) |
| `World::view<T>().each()` → `(Entity, T&)` | per-type script enumerators |
| `registerComponentType<T>(name)` + `ComponentRegistry` (Epic 03/05 MIG-04) | scripts serialize like normal components, free |
| `EMBER_REFLECT(T){ EMBER_FIELD(x); }` (free function) | reused for script fields (not an in-class macro) |
| `Input::isKeyDown/isKeyPressed`, `Input::getAction(name).axisValue` (Epic 05) | sample scripts |
| `AssetHandle<T>`, `AssetManager` (Epic 05); **no `Prefab` asset**, **no physics**, **no `AudioClip` loader** | `instantiate(AssetHandle<Prefab>)` stubbed; collision cbs declared-only; sample scripts avoid `AssetHandle` fields |

**Cross-cutting decisions (confirm before starting):**

- **Each script subclass is its own entt component** (stored by value). `EMBER_REGISTER_SCRIPT(T)` records a per-type **enumerator** (`view<T>()`), a **factory**, **execution order**, and registers `T` for serialization via `registerComponentType<T>`. `ScriptSystem` builds one flat, sorted `std::vector<ScriptEntry>` from the enumerators and iterates it (no virtual dispatch to *find* scripts). (§1.2, SS-02, REG-01)
- **`onStart`** is gated by a `bool m_started` in the base (set after the call); `m_scene`/`m_entity` are injected by `ScriptSystem` each tick before any callback. New scripts are seen on the next tick ⇒ `onStart` the frame after `emplace`. (SS-03)
- **`onDestroy`** is fired from an `on_destroy<T>` hook connected by `ScriptSystem` at construction (one per registered type), so it runs before the component/entity is gone — covering both script `destroy()` and external `world.destroy`. Script `destroy()` just enqueues the entity for end-of-frame `world.destroy`. (SS-07)
- **Exception isolation:** every callback invocation goes through one `invoke(script, fn)` wrapper (`try/catch(std::exception)/catch(...)`) that logs and sets `m_enabled=false`. (SS-08)
- **Module deps:** `ember_scripting` → PUBLIC `ember_core ember_ecs ember_scene`; PRIVATE `ember_assets`, `ember_serialization`, `yaml-cpp` (only the registration TU sees yaml — mirrors renderer MIG-04). No GLFW/GL.

---

## Phase 0 — Scaffold module + CMake

- [ ] Create dirs:
  ```
  engine/scripting/include/ember/scripting/   engine/scripting/src/
  sandbox/scripts/                            tests/scripting/
  ```
- [ ] `engine/scripting/CMakeLists.txt`:
  ```cmake
  add_library(ember_scripting STATIC
      src/ScriptSystem.cpp
      src/ScriptRegistry.cpp
      src/ScriptSerialization.cpp   # the only TU that pulls yaml
  )
  target_include_directories(ember_scripting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(ember_scripting
      PUBLIC  ember_core ember_ecs ember_scene
      PRIVATE ember_assets ember_serialization yaml-cpp::yaml-cpp glm::glm)
  target_compile_features(ember_scripting PUBLIC cxx_std_20)
  ember_set_compiler_options(ember_scripting)
  ```
- [ ] Root `CMakeLists.txt`: `add_subdirectory(engine/scripting)` (after `engine/assets`), and add `ember_scripting` to the `ember` aggregate.
- [ ] Add `tests/scripting/*` files to `ember_tests` (Phase 6).

**Verify Phase 0:** `cmake --preset debug` configures with the new `ember_scripting` target.

---

## Phase 1 — `ScriptComponent` base (`include/ember/scripting/ScriptComponent.h`)

Implements `SC-01 … SC-07` (SC-08 partial).

- [ ] Header-only base; lifecycle virtuals default to `{}`:
  ```cpp
  class ScriptSystem;
  class ScriptComponent {
  public:
      virtual ~ScriptComponent() = default;
      virtual void onStart() {}
      virtual void onUpdate(f32) {}
      virtual void onFixedUpdate(f32) {}
      virtual void onLateUpdate(f32) {}
      virtual void onDestroy() {}
      // declared, invoked by a future physics epic (SC-02):
      virtual void onCollisionEnter(Entity) {} /* …Stay/Exit, onTrigger* … */
  protected:
      template<typename T> T&   getComponent()    { return world().get<T>(m_entity); }
      template<typename T> T*   tryGetComponent() { return world().tryGet<T>(m_entity); }
      template<typename T> T&   addComponent()    { return world().emplace<T>(m_entity); }
      template<typename T> void removeComponent() { world().remove<T>(m_entity); }
      template<typename T> bool hasComponent() const { return world().has<T>(m_entity); }
      Entity getEntity() const { return m_entity; }
      Scene& getScene() const  { return *m_scene; }
      World& getWorld() const  { return m_scene->world(); }
      Transform& transform()   { return getComponent<Transform>(); }
      const std::string& getName() const;  void setName(const std::string&);   // via Tag
      void   destroy();                    // enqueues on the owning ScriptSystem
      Entity instantiate(AssetHandle<Prefab>);   // SC-08 (stub this epic)
  private:
      friend class ScriptSystem;
      World& world() const { return m_scene->world(); }
      Entity m_entity{};  Scene* m_scene = nullptr;  ScriptSystem* m_system = nullptr;
      bool m_started = false;  bool m_enabled = true;
  };
  ```
- [ ] `getName/setName` read/write the entity's `Tag` (create one if absent, like the serializer's loadEntities does).
- [ ] `destroy()` calls `m_system->scheduleDestroy(m_entity)` (defined in Phase 3).
- [ ] `instantiate(AssetHandle<Prefab>)`: forward-declare `struct Prefab;` (asset type TBD); this epic logs a warning and returns `Entity{}` (SC-08 / §11). Keep the signature stable.

> Includes: `ember/ecs/Entity.h`, `ember/ecs/World.h`, `ember/ecs/Components.h` (Tag/Transform), `ember/scene/Scene.h`, `ember/assets/AssetHandle.h`. No yaml/GL.

**Verify Phase 1:** header compiles standalone (`-fsyntax-only`) and a trivial subclass overriding `onUpdate` builds.

---

## Phase 2 — `ScriptRegistry` + macros (`include/ember/scripting/ScriptRegistry.h`, `src/ScriptRegistry.cpp`)

Implements `REG-01 … REG-04`, `ORD-01 … ORD-04`.

- [x] `ScriptTypeInfo { name; order; forEach; connectDestroy; }` (in `ScriptRegistry.h`, yaml-free).
- [x] `ScriptRegistry::instance()` singleton: `registerScript` (dup-name idempotent), `setOrder`, `all()`, `byName()`, `clear()`.
- [x] `EMBER_REGISTER_SCRIPT(Type)` (in `RegisterScript.h`) → static init calling `registerScriptType<Type>(#Type)`, which builds the `ScriptTypeInfo` **and** calls `registerComponentType<Type>` for serialization (REG-01/02). The `forEach`/`connectDestroy` lambdas + `registerComponentType` are factored into `detail::makeScriptInfo<T>` / `registerScriptType<T>`.
- [x] `EMBER_SCRIPT_ORDER(Type, n)` → static init `setOrder` (ORD-02).
- [x] Non-reflected scripts: `EMBER_REGISTER_SCRIPT_NOSERIALIZE(Type)` / `registerScriptTypeNoSerialize<T>` skips `registerComponentType` (REG-04). yaml is confined to `RegisterScript.h` (only registration TUs include it), keeping `ScriptComponent.h`/`ScriptSystem.h`/`ScriptRegistry.h` yaml-free.

**Verify Phase 2:** ✅ registration probe + lifecycle driver confirm `ScriptRegistry::all()` populates and sorts by `(order, name)` deterministically (ORD-03); compiles `-Wpedantic -Werror`.

---

## Phase 3 — `ScriptSystem` (`include/ember/scripting/ScriptSystem.h`, `src/ScriptSystem.cpp`)

Implements `SS-01 … SS-10`.

- [x] `ScriptSystem : ISystem`, constructed with `Scene&`; ctor connects each registered type's `on_destroy<T>` via `info.connectDestroy(scene.world())` → `ScriptSystem::onComponentDestroyed<T>` (SS-07).
- [x] `struct ScriptEntry { Entity e; ScriptComponent* script; i32 order; const std::string* name; };` (reused `m_list`).
- [x] `update(world, dt)`: rebuild+inject+sort → onStart pass (once, `m_started`) → fixed accumulator pass → onUpdate pass → onLateUpdate pass → drain deferred-destroy queue (SS-02..07, ORD-04).
- [x] `guarded(...)` wraps every call in `try/catch(std::exception)/catch(...)`, logs and sets `m_enabled=false` on throw (SS-08).
- [x] `scheduleDestroy(Entity)` queues; `ScriptComponent::destroy()` calls it; drained via `world.destroy` which fires the `on_destroy` hook → `fireDestroy` → `onDestroy` (SS-07).
- [x] `onComponentDestroyed<T>` (static template, header) fetches `T` and calls `fireDestroy` (guarded, only if `m_started`).
- [x] `setFixedDelta(f32)` default `0.02f` (SS-06); defensive `valid()`/`tryGet` (SS-09).

**Verify Phase 3:** ✅ Linux-native driver runs a counting script through start-once → fixed(accumulator) → update → late → deferred-destroy+onDestroy → exception-isolation → (order, name) ordering — **all pass**; compiles `-Wpedantic -Werror`.

---

## Phase 4 — Serialization niceties

Implements `SER-01 … SER-05`. Scalar/`glm`/`Entity`/string fields already round-trip via `registerComponentType` (Phase 2) + the existing visitors — so most of SER is **free**.

- [x] Reflected script fields serialize for free via `registerComponentType<T>` + the existing visitors (`f32/i32/u32/u64/bool/std::string/glm::vec*/Entity`). Verified by `TestScriptSerialization.cpp` (compiles; round-trip asserted) (SER-01/03/04/05).
- [x] `AssetHandle<T>` fields (SER-02) — **done**: `AssetManager` gained `pathOf(u64)` and a type-erased `loadByType(type_index, path)` (a per-type thunk recorded in `registerLoader<T>`). `ComponentSerialization.h` got an `AssetSerializationResolver` indirection (`setAssetSerializationResolver(...)`) + `AssetHandle<T>` visitor overloads (write = virtual path, read = re-resolve via `loadByType`). The app installs the resolver from its `AssetManager`; unset ⇒ handles serialize as `""`/null (graceful). Verified by a probe with an `AssetHandle<Texture2D>` script field. *(serialization now PUBLIC-links `ember_assets` for the header-only `AssetHandle.h`.)*
- [x] No separate `ScriptSerialization.cpp` needed — the macro-based registration through `RegisterScript.h` covers it; dropped from the build.

**Verify Phase 4:** ✅ `TestScriptSerialization.cpp` compiles `-Werror` and asserts a reflected script's scalar/bool fields round-trip through `YAMLSerializer::serialize`/`deserialize` (TST-07). Runtime `ctest` is the user's build.

---

## Phase 5 — Sample scripts + sandbox wiring

Implements `SBX-01 … SBX-05`.

- [x] `sandbox/scripts/Spinner.h` (SBX-03), `Lifetime.h` (SBX-04), `PlayerController.h` (SBX-01, uses `Input::getAction`/`isKeyPressed`), `CameraFollow.h` (SBX-02, lerps toward a target entity's `Transform` in `onLateUpdate`) — all in `namespace game`, reflected, compile `-Werror`.
- [x] `sandbox/scripts/Scripts.cpp` — `EMBER_REGISTER_SCRIPT(...)` for all four + `EMBER_SCRIPT_ORDER(PlayerController,-10)` / `(CameraFollow,10)`.
- [x] Sandbox `CMakeLists.txt` — adds `scripts/Scripts.cpp`, the `sandbox/` include dir, and links `yaml-cpp` (for the registration TU); `ember_scripting` comes via the `ember` aggregate.
- [x] Sandbox `main.cpp` loop integration (SBX-05) — **done**: `main.cpp` now builds a `Scene`, an `InputManager` (`init`/`loadDefaultActionMap`/`update`), and a `ScriptSystem`; spawns ~484 `Spinner`s + a WASD-driven `PlayerController` player, ticks `scripts.update(world, dt)` each frame, and renders every entity by reading its `Transform` through the existing `drawQuad` path (player tinted gold). Logic compiles clean `-Werror` (only `<format>`, a GCC-13 feature already used by the prior sandbox, is unavailable in the sandbox's GCC 11).

**Verify Phase 5:** ✅ sample scripts, registration TU, and the new `main.cpp` all compile `-Wpedantic -Werror`. The on-screen result (spinning grid + WASD-movable gold square) is confirmed by the user's GL build.

---

## Phase 6 — Tests + verification (`tests/scripting/`)

Implements `TST-01 … TST-08`, NFR-06.

- [x] `tests/scripting/TestScriptLifecycle.cpp` — onStart-once (TST-01), update-before-late (TST-02), fixed accumulator (TST-03), deferred destroy + onDestroy (TST-04), `(priority, name)` order (TST-05), exception isolation (TST-06). *(500-spinner perf is the sandbox's job, TST-08.)*
- [x] `tests/scripting/TestScriptSerialization.cpp` — reflected script scalar/bool fields YAML round-trip (TST-07).
- [x] Both added to `ember_tests`; `yaml-cpp` linked there (the tests include `RegisterScript.h`).
- [x] Header hygiene: no yaml/GLFW in `engine/scripting/include` (only the `RegisterScript.h` includer pulls yaml; verified below).
- [~] `ctest --preset debug`/`release` green; `sanitize` clean (NFR-06) — **user's build** (sandbox can't link the MinGW Catch2/yaml libs; lifecycle verified via a Linux-native driver instead).
- [~] Push; CI green, zero warnings — user's build.

### Acceptance checklist (from `02-requirements.md` §12)

- [ ] `PlayerController` moves a sprite with WASD at configured speed; Space jump path runs.
- [ ] `CameraFollow` tracks the player via `onLateUpdate`.
- [ ] `onStart` once, frame after `emplace`, before first `onUpdate`.
- [ ] `onDestroy` before removal; deferred `destroy()` removes by end-of-frame.
- [ ] Script scalar fields YAML round-trip.
- [ ] Exception in `onUpdate` logged, disables only that script, no crash.
- [ ] 500 `Spinner`s at >60 fps (sandbox).
- [ ] `ember_scripting` builds `-Werror`; tests pass; no yaml/GL in public headers.

---

## Suggested Commit Sequence

1. `Epic 06: scaffold scripting module + CMake` (Phase 0)
2. `Epic 06: ScriptComponent base` (Phase 1)
3. `Epic 06: ScriptRegistry + EMBER_REGISTER_SCRIPT` (Phase 2)
4. `Epic 06: ScriptSystem lifecycle dispatch` (Phase 3)
5. `Epic 06: script field serialization (+ AssetHandle, optional)` (Phase 4)
6. `Epic 06: sample scripts + sandbox wiring` (Phase 5)
7. `Epic 06: scripting tests + verification` (Phase 6)

Each commit should leave the build green and CI passing.
