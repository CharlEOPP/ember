# Epic 03 — ECS & Scene: Implementation Plan

**Status:** Not started  
**Document:** 03-implementation-plan.md  
**Refs:** epic.md, 02-requirements.md, 01-vision.md §5/§6/§10/§12

Work through the phases in order. Each phase produces a buildable, testable state. Do not proceed until all checkboxes in the current phase are ticked and the stated verification passes. Requirement IDs (e.g., `EVT-01`) refer to `02-requirements.md`.

**Tech stack this epic builds on (already fetched in `cmake/Dependencies.cmake`):**

| Dependency | Version / target | Used for |
|------------|------------------|----------|
| EnTT | `v3.13.2` → `EnTT::EnTT` | registry, views, signals (`entt::sigh`/`sink`), dispatcher |
| yaml-cpp | `0.8.0` → `yaml-cpp::yaml-cpp` | scene/prefab (de)serialization |
| glm | `1.0.1` → `glm::glm` | `vec3`/`vec4`/`mat4` in components |
| Catch2 | `v3.6.0` → `Catch2::Catch2WithMain` | tests |
| (core) | `ember_core` | `Result`, `EMBER_LOG_*`, `EMBER_ASSERT`, `Types.h`, `UUID` |

**Cross-cutting decisions (confirm before starting):**

- **Entity width.** EnTT's default `entt::entity` is 32-bit. To satisfy `ECS-01` (64-bit ids) we define `ENTT_ID_TYPE=std::uint64_t` as a **PUBLIC** compile definition on `ember_ecs` so every translation unit that touches EnTT agrees on the id type. (Mismatched `ENTT_ID_TYPE` across TUs is an ODR violation — keep it in one place.)
- **EnTT in headers.** `World::emplace<T>`, `view<T...>`, and `on_construct<T>` are templates and therefore must live in headers, which forces `<entt/entt.hpp>` to be included by `World.h`. This relaxes `NFR-04` for EnTT specifically: EnTT is included (as a `SYSTEM` dependency) but the `entt::registry` instance stays private and callers never manipulate it directly. **yaml-cpp remains fully hidden** behind non-template functions implemented in `.cpp` (honour `SER-11`/`NFR-04` for YAML). Flag if you'd rather pImpl the World to hide EnTT entirely.
- **Build the modules in dependency order:** events → ecs → scene → serialization. `serialization` depends on `ecs` (ComponentRegistry) and `scene`.

---

## Phase 0 — Module Scaffolding & CMake Wiring

Goal: four new empty static-lib targets configure and link; `cmake --preset debug` succeeds with no new sources yet.

### 0.1 Create directory trees

- [x] Create:
  ```
  engine/events/include/ember/events/
  engine/events/src/
  engine/ecs/include/ember/ecs/
  engine/ecs/src/
  engine/scene/include/ember/scene/
  engine/scene/src/
  engine/scene/systems/
  engine/serialization/include/ember/serialization/
  engine/serialization/src/
  tests/events/  tests/ecs/  tests/scene/  tests/serialization/
  ```

### 0.2 Wire root `CMakeLists.txt`

- [x] Add the new subdirectories (after `engine/platform`, before `sandbox`):
  ```cmake
  add_subdirectory(engine/events)
  add_subdirectory(engine/ecs)
  add_subdirectory(engine/scene)
  add_subdirectory(engine/serialization)
  ```
- [x] Extend the aggregate interface target:
  ```cmake
  target_link_libraries(ember INTERFACE
      ember_core
      ember_platform
      ember_renderer_api
      ember_events
      ember_ecs
      ember_scene
      ember_serialization
  )
  ```

### 0.3 Module `CMakeLists.txt` files

- [x] `engine/events/CMakeLists.txt`:
  ```cmake
  add_library(ember_events STATIC src/EventBus.cpp)
  target_include_directories(ember_events PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(ember_events PUBLIC ember_core EnTT::EnTT)
  target_compile_features(ember_events PUBLIC cxx_std_20)
  ember_set_compiler_options(ember_events)
  ```
- [x] `engine/ecs/CMakeLists.txt`:
  ```cmake
  add_library(ember_ecs STATIC
      src/World.cpp
      src/ComponentRegistry.cpp
      src/Components.cpp        # holds the EMBER_REFLECT registrations
  )
  target_include_directories(ember_ecs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(ember_ecs PUBLIC ember_core EnTT::EnTT glm::glm)
  # 64-bit entity ids — PUBLIC so all consumers compile EnTT with the same id type
  target_compile_definitions(ember_ecs PUBLIC ENTT_ID_TYPE=std::uint64_t)
  target_compile_features(ember_ecs PUBLIC cxx_std_20)
  ember_set_compiler_options(ember_ecs)
  ```
- [x] `engine/scene/CMakeLists.txt`:
  ```cmake
  add_library(ember_scene STATIC
      src/Scene.cpp
      src/SceneManager.cpp
      systems/TransformSystem.cpp
  )
  target_include_directories(ember_scene
      PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/systems)
  target_link_libraries(ember_scene PUBLIC ember_core ember_ecs ember_events glm::glm)
  target_compile_features(ember_scene PUBLIC cxx_std_20)
  ember_set_compiler_options(ember_scene)
  ```
- [x] `engine/serialization/CMakeLists.txt` (yaml-cpp is **PRIVATE** so it never leaks into consumers):
  ```cmake
  add_library(ember_serialization STATIC
      src/YAMLSerializer.cpp
      src/YAMLDeserializer.cpp
  )
  target_include_directories(ember_serialization PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(ember_serialization
      PUBLIC  ember_core ember_ecs ember_scene
      PRIVATE yaml-cpp::yaml-cpp)
  target_compile_features(ember_serialization PUBLIC cxx_std_20)
  ember_set_compiler_options(ember_serialization)
  ```

  > If a CI runner resolves yaml-cpp's target as `yaml-cpp` (no namespace alias on some 0.8 builds), use `$<IF:$<TARGET_EXISTS:yaml-cpp::yaml-cpp>,yaml-cpp::yaml-cpp,yaml-cpp>` or mirror the `if(TARGET ...)` fallback used for spdlog in `engine/core/CMakeLists.txt`.

**Verify Phase 0:**
```sh
cmake --preset debug
# Expected: "Configuring done" / "Generating done", new targets present, no errors.
```

---

## Phase 1 — Event System (`engine/events/`)

Implements `EVT-01 … EVT-18`.

### 1.1 `Events.h` — built-in event types

- [x] Create `engine/events/include/ember/events/Events.h` (`EVT-12 … EVT-15`):
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include <string>

  namespace ember {

  // Window (supersede the stub structs from Epic 02's platform/Event.h)
  struct WindowResizeEvent { u32 width, height; };
  struct WindowCloseEvent  {};

  // Input (types only; produced by the Input epic, not here)
  struct KeyPressedEvent   { i32 key; i32 mods; bool repeat; };
  struct KeyReleasedEvent  { i32 key; i32 mods; };
  struct MouseMovedEvent   { f32 x, y; };
  struct MouseButtonEvent  { i32 button; i32 action; i32 mods; };
  struct MouseScrollEvent  { f32 xOffset, yOffset; };

  // Scene
  struct SceneLoadedEvent   { std::string name; };
  struct SceneUnloadedEvent { std::string name; };

  } // namespace ember
  ```

### 1.2 `EventBus.h` / `EventBus.cpp` — deferred queue

- [x] Create `engine/events/include/ember/events/EventBus.h` (`EVT-01 … EVT-11`):
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include "ember/core/Log.h"
  #include <functional>
  #include <typeindex>
  #include <unordered_map>
  #include <vector>
  #include <deque>
  #include <memory>

  namespace ember {

  class EventBus;

  // RAII handle: unsubscribes on destruction (EVT-02..04)
  class SubscriptionToken {
  public:
      SubscriptionToken() = default;
      SubscriptionToken(EventBus* bus, std::type_index type, u64 id)
          : m_bus(bus), m_type(type), m_id(id) {}
      ~SubscriptionToken();
      SubscriptionToken(const SubscriptionToken&)            = delete;
      SubscriptionToken& operator=(const SubscriptionToken&) = delete;
      SubscriptionToken(SubscriptionToken&& o) noexcept { *this = std::move(o); }
      SubscriptionToken& operator=(SubscriptionToken&& o) noexcept;
  private:
      EventBus*       m_bus  = nullptr;   // null => moved-from / empty (EVT-04)
      std::type_index m_type = typeid(void);
      u64             m_id   = 0;
  };

  class EventBus {
  public:
      template<typename T, typename... Args>
      void post(Args&&... args) {                       // EVT-01
          auto& ch = channel<T>();
          if (ch.queue.size() >= ch.capacity) {         // EVT-09
              EMBER_LOG_WARN("EventBus: ring buffer for '{}' full ({}), dropping",
                             typeid(T).name(), ch.capacity);
              return;
          }
          ch.queue.emplace_back(T{std::forward<Args>(args)...});
      }

      template<typename T>
      [[nodiscard]] SubscriptionToken subscribe(std::function<void(const T&)> fn) { // EVT-02
          auto& ch = channel<T>();
          const u64 id = ++ch.nextId;
          ch.subs.push_back({id, [fn = std::move(fn)](const void* e) {
              fn(*static_cast<const T*>(e));
          }});
          return SubscriptionToken{this, std::type_index(typeid(T)), id};
      }

      template<typename T>
      void reserve(usize capacity) { channel<T>().capacity = capacity; } // EVT-08

      void dispatch();                                  // EVT-05/06 (impl in .cpp)

  private:
      friend class SubscriptionToken;
      struct Sub { u64 id; std::function<void(const void*)> call; };
      struct Channel {
          usize                 capacity = 256;          // EVT-07
          std::deque<std::any>  queue;                   // typed payloads
          std::vector<Sub>      subs;
          u64                   nextId = 0;
          std::function<void(Channel&)> drain;           // type-erased dispatch
      };

      template<typename T> Channel& channel();           // creates+wires drain on first use
      void unsubscribe(std::type_index type, u64 id);

      std::unordered_map<std::type_index, Channel> m_channels;
  };

  } // namespace ember
  ```

  > Implementation note: `std::any` keeps the bus header-only-friendly and type-safe; for the hot input events you can swap a channel's storage to a fixed `std::vector<T>` ring later without changing the public API. The per-`Channel` `drain` lambda (installed in `channel<T>()`) casts payloads back to `T` so `dispatch()` needs no template knowledge.

- [x] Create `engine/events/src/EventBus.cpp` implementing `dispatch()` (drain every channel FIFO, clear queues — `EVT-05/06`), `unsubscribe()`, and `SubscriptionToken`'s destructor/move (`EVT-03/04`). Document the main-thread-only contract at the top of the header (`EVT-11`).

### 1.3 `Signal.h` — immediate dispatch (EnTT)

- [x] Create `engine/events/include/ember/events/Signal.h` (`EVT-16 … EVT-18`) — implemented as a lambda-friendly `std::function` dispatcher; EnTT-native signals are used for ECS lifecycle hooks via `World` (see note below):
  ```cpp
  #pragma once
  #include <entt/signal/sigh.hpp>
  #include <utility>

  namespace ember {

  // Thin wrapper over entt::sigh/sink for immediate synchronous callbacks.
  template<typename T>
  class Signal {
  public:
      auto sink() { return entt::sink{m_signal}; }     // connect handlers via .connect<...>()
      template<typename F> void connect(F&& f) { entt::sink{m_signal}.template connect<>(std::forward<F>(f)); }
      void fire(const T& e) { m_signal.publish(e); }    // EVT-17
  private:
      entt::sigh<void(const T&)> m_signal;
  };

  } // namespace ember
  ```

### 1.4 Tests

- [x] `tests/events/TestEventBus.cpp` — `EventBus_PostDispatchDelivers`, `EventBus_DeferredNotImmediate`, `EventBus_TokenAutoUnsubscribe`, `EventBus_MultipleSubscribers`, `EventBus_RingBufferCapacity`.
- [x] `tests/events/TestSignal.cpp` — `Signal_ImmediateInvoke`.
- [ ] Add these files to the `ember_tests` source list and link `ember_events` (see Phase 6.1).

**Verify Phase 1:**
```sh
cmake --build --preset debug --target ember_events
ctest --preset debug -R "EventBus|Signal"
```

---

## Phase 2 — ECS Core (`engine/ecs/`)

Implements `ECS-01 … ECS-20`.

### 2.1 `Entity.h`

- [x] Create `engine/ecs/include/ember/ecs/Entity.h` (`ECS-01..03`):
  ```cpp
  #pragma once
  #include <entt/entity/entity.hpp>   // ENTT_ID_TYPE set to uint64_t via CMake (PUBLIC)
  #include <entt/entity/fwd.hpp>

  namespace ember {
  using Entity = entt::entity;                 // 64-bit (index + generation)
  inline constexpr Entity NULL_ENTITY = entt::null;
  } // namespace ember
  ```

### 2.2 `World.h` / `World.cpp`

- [x] Create `engine/ecs/include/ember/ecs/World.h` (`ECS-04 … ECS-15`) — note: `emplace` returns `decltype(auto)` so empty (tag) components work; `size()` uses a manual alive-counter; `registry()` escape hatch added:
  ```cpp
  #pragma once
  #include "ember/ecs/Entity.h"
  #include "ember/core/Assert.h"
  #include <entt/entt.hpp>

  namespace ember {

  class World {
  public:
      Entity create() { return m_reg.create(); }                       // ECS-05
      void   destroy(Entity e) {                                        // ECS-06
          EMBER_ASSERT(m_reg.valid(e), "destroy() on invalid entity");
          if (m_reg.valid(e)) m_reg.destroy(e);
      }
      bool   valid(Entity e) const { return m_reg.valid(e); }          // ECS-14
      void   clear() { m_reg.clear(); }                                // ECS-15

      template<typename T, typename... Args>
      T& emplace(Entity e, Args&&... args) {                           // ECS-07
          EMBER_ASSERT(!m_reg.all_of<T>(e), "emplace<T>: component already present");
          return m_reg.emplace<T>(e, std::forward<Args>(args)...);
      }
      template<typename T> void remove(Entity e) { m_reg.remove<T>(e); } // ECS-08
      template<typename T> T&   get(Entity e) {                         // ECS-09
          EMBER_ASSERT(m_reg.all_of<T>(e), "get<T>: component absent");
          return m_reg.get<T>(e);
      }
      template<typename T> T*   tryGet(Entity e) { return m_reg.try_get<T>(e); } // ECS-10
      template<typename... T> bool has(Entity e) const { return m_reg.all_of<T...>(e); } // ECS-11
      template<typename... T> auto view() { return m_reg.view<T...>().each(); }  // ECS-12 (structured bindings)

      template<typename T> auto on_construct() { return m_reg.on_construct<T>(); } // ECS-13
      template<typename T> auto on_destroy()   { return m_reg.on_destroy<T>();  }

  private:
      entt::registry m_reg;   // wrapped, never exposed (ECS-04 / NFR-05)
  };

  } // namespace ember
  ```
  > `World.cpp` can be empty initially (everything is header templates) but exists so the target has a TU; put any non-template helpers here as they appear.

### 2.3 `SystemScheduler.h` / `.cpp`

- [x] Create `engine/ecs/include/ember/ecs/SystemScheduler.h` (`ECS-16 … ECS-20`):
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include "ember/ecs/World.h"
  #include <array>
  #include <memory>
  #include <vector>

  namespace ember {

  enum class Phase { PreUpdate, Update, PostUpdate, Render, Debug, Count }; // ECS-16

  struct ISystem { virtual ~ISystem() = default; virtual void update(World&, f32 dt) = 0; };

  class SystemScheduler {
  public:
      template<typename T, typename... Args>
      T& addSystem(Phase phase, Args&&... args) {                        // ECS-17
          auto sys = std::make_unique<T>(std::forward<Args>(args)...);
          T& ref = *sys;
          m_phases[static_cast<usize>(phase)].push_back(std::move(sys));
          return ref;
      }
      void runPhase(Phase phase, World& world, f32 dt) {                  // ECS-18
          for (auto& s : m_phases[static_cast<usize>(phase)]) s->update(world, dt);
      }
  private:
      std::array<std::vector<std::unique_ptr<ISystem>>, static_cast<usize>(Phase::Count)> m_phases;
  };

  } // namespace ember
  ```

### 2.4 Tests

- [x] `tests/ecs/TestWorld.cpp` — `World_CreateDestroy`, `World_ComponentCRUD`, `World_ViewIteration`, `World_LifecycleHooks`.
- [x] `tests/ecs/TestScheduler.cpp` — `Scheduler_PhaseOrder`.

**Verify Phase 2:**
```sh
cmake --build --preset debug --target ember_ecs
ctest --preset debug -R "World|Scheduler"
```

---

## Phase 3 — Reflection, ComponentRegistry & Built-in Components

Implements `REF-01 … REF-07`, `CMP-01 … CMP-07`.

### 3.1 `Reflect.h` — field enumeration macros

- [x] Create `engine/ecs/include/ember/ecs/Reflect.h` (`REF-01/02/07`). Template-visitor design (`reflect()` overload per type + `EMBER_FIELD`):
  ```cpp
  #pragma once
  #include <string_view>

  namespace ember {

  // A visitor receives (name, member-pointer). The serializer supplies a visitor
  // that reads/writes YAML; the inspector (later epic) supplies one that draws UI.
  template<typename T, typename Visitor>
  void reflect(T& obj, Visitor&& v);   // specialised per component via EMBER_REFLECT

  } // namespace ember

  // Usage (in a .cpp):
  //   EMBER_REFLECT(Transform) { EMBER_FIELD(position); EMBER_FIELD(rotation); EMBER_FIELD(scale); }
  #define EMBER_REFLECT(T)                                           \
      template<typename V> void ::ember::reflect(T& obj, V&& visit)
  #define EMBER_FIELD(member)  visit(#member, obj.member)
  ```
  > This is the lightweight, compile-time field walk the vision §12 calls for — no RTTI, no external reflection lib. Keep `reflect()` definitions in `.cpp` so headers stay clean. If you prefer a registry of `std::function` field accessors instead of a template visitor, note it now; the serializer in Phase 5 is written against this `reflect()` shape.

### 3.2 `ComponentRegistry.h` / `.cpp`

- [x] Create `engine/ecs/include/ember/ecs/ComponentRegistry.h` (`REF-03 … REF-06`) — serialize/deserialize use `void*` (YAML hidden); deserialize also takes the id-remap map:
  ```cpp
  #pragma once
  #include "ember/ecs/Entity.h"
  #include <string>
  #include <string_view>
  #include <typeindex>
  #include <unordered_map>
  #include <functional>

  namespace ember {
  class World;

  struct ComponentMeta {
      std::string     name;
      std::type_index type = typeid(void);
      // Serialize/deserialize a single component on an entity (wired in Phase 5).
      std::function<void(const World&, Entity, /*YAML out*/ void*)> serialize;
      std::function<void(World&, Entity, /*YAML in*/ const void*)>  deserialize;
      std::function<void(World&, Entity)>                           drawUI;   // REF-04: reserved, null this epic
  };

  class ComponentRegistry {
  public:
      static ComponentRegistry& instance();
      void                 add(ComponentMeta meta);                 // REF-06: assert on dup
      const ComponentMeta* byName(std::string_view name) const;     // REF-05
      const ComponentMeta* byType(std::type_index type) const;      // REF-05
      const std::unordered_map<std::string, ComponentMeta>& all() const { return m_byName; }
  private:
      std::unordered_map<std::string, ComponentMeta>      m_byName;
      std::unordered_map<std::type_index, std::string>    m_typeToName;
  };

  // Registration helper (one static instance per component, see Components.cpp)
  template<typename T> struct ComponentRegistrar { explicit ComponentRegistrar(ComponentMeta m); };
  } // namespace ember
  ```
  > The `void*` YAML params keep yaml-cpp out of this header (`NFR-04`); the Phase-5 serializer casts them to `YAML::Emitter*` / `const YAML::Node*` inside the `.cpp`.

### 3.3 `Components.h` + `Components.cpp`

- [x] Create `engine/ecs/include/ember/ecs/Components.h` (`CMP-01 … CMP-06`) — also holds the `EMBER_REFLECT` field walks (header-visible for the serializer):
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include "ember/ecs/Entity.h"
  #include <glm/glm.hpp>
  #include <string>
  #include <vector>

  namespace ember {
  struct Transform      { glm::vec3 position{0}; f32 rotation = 0.0f; glm::vec3 scale{1}; };
  struct WorldTransform { glm::mat4 matrix{1.0f}; bool dirty = true; };
  struct Tag            { std::string name; std::string layer; };
  struct Parent         { Entity parent = NULL_ENTITY; };
  struct Children       { std::vector<Entity> children; };
  } // namespace ember
  ```
- [x] Create `engine/ecs/src/Components.cpp` (`CMP-07`, `REF-01`) — reflection is header-defined; registry population (with YAML callbacks) is centralized in the serialization module's init to keep yaml-cpp out of ecs:
  ```cpp
  #include "ember/ecs/Components.h"
  #include "ember/ecs/Reflect.h"
  // EMBER_REFLECT(Transform) { EMBER_FIELD(position); EMBER_FIELD(rotation); EMBER_FIELD(scale); }
  // EMBER_REFLECT(Tag)       { EMBER_FIELD(name);     EMBER_FIELD(layer); }
  // EMBER_REFLECT(Parent)    { EMBER_FIELD(parent); }
  // EMBER_REFLECT(Children)  { EMBER_FIELD(children); }
  // WorldTransform is runtime-derived (cached) — NOT serialized.
  // ... plus ComponentRegistrar<Transform>{{...}} static instances to populate the registry.
  ```
  > Decision to confirm: `WorldTransform` is excluded from serialization (it's a cache rebuilt by `TransformSystem`). Only `Transform`/`Tag`/`Parent`/`Children` are persisted.

### 3.4 Tests

- [x] `tests/ecs/TestReflection.cpp` — `Reflection_FieldWalk` (visits expected names) and `Reflection_RegistryLookup` (byName/byType, duplicate rejected).

**Verify Phase 3:**
```sh
cmake --build --preset debug --target ember_ecs
ctest --preset debug -R "Reflection"
```

---

## Phase 4 — Scene, Hierarchy & SceneManager (`engine/scene/`)

Implements `SCN-01 … SCN-13`, `SCM-01 … SCM-06`.

### 4.1 `Scene.h` / `Scene.cpp`

- [x] Create `engine/scene/include/ember/scene/Scene.h` (`SCN-01 … SCN-10`) — `create()` also adds Transform + WorldTransform; added `markTransformDirty()`:
  ```cpp
  #pragma once
  #include "ember/ecs/World.h"
  #include "ember/ecs/Components.h"
  #include <glm/glm.hpp>
  #include <string>
  #include <string_view>
  #include <vector>

  namespace ember {

  struct SceneSettings { glm::vec4 backgroundColor{0.1f, 0.1f, 0.15f, 1.0f}; };

  class Scene {
  public:
      explicit Scene(std::string name) : m_name(std::move(name)) {}

      World&       world()       { return m_world; }
      const World& world() const { return m_world; }
      const std::string& name() const { return m_name; }
      SceneSettings& settings() { return m_settings; }

      Entity create();                                   // SCN-03 (wraps world.create + Tag)
      void   destroy(Entity e);                          // SCN-03 (detach from hierarchy)

      Entity              findByName(std::string_view name);   // SCN-04
      std::vector<Entity> findByTag(std::string_view layer);   // SCN-05

      void   setParent(Entity child, Entity parent);     // SCN-07 (+cycle guard SCN-09)
      Entity getParent(Entity child);                    // SCN-08
      glm::mat4 getWorldTransform(Entity e);             // SCN-10 (resolve dirty chain)

  private:
      bool isAncestor(Entity maybeAncestor, Entity of);  // cycle check
      std::string   m_name;
      World         m_world;
      SceneSettings m_settings;
  };

  } // namespace ember
  ```
- [x] Implement `Scene.cpp`. Key behaviours:
  - `setParent`: remove `child` from any previous parent's `Children`, set `Parent{parent}`, push to new parent's `Children`; mark child's `WorldTransform.dirty` (`SCN-07/09/11`).
  - `getWorldTransform`: walk up to the nearest clean ancestor, recompute downward, cache into `WorldTransform.matrix`, clear dirty (`SCN-10/13`).
  - `findByName`/`findByTag`: iterate `world().view<Tag>()`.

### 4.2 `TransformSystem`

- [x] Create `engine/scene/systems/TransformSystem.h` + `.cpp` (`SCN-12`), an `ISystem` registered in `Phase::PreUpdate`. Recomputes dirty `WorldTransform`s parent-first via recursive `resolveWorldTransform`; local matrix = `translate * rotateZ * scale`.

### 4.3 `SceneManager.h` / `.cpp`

- [x] Create `engine/scene/include/ember/scene/SceneManager.h` + `.cpp` (`SCM-01 … SCM-06`) — uses an injected `setLoader()` hook so scene doesn't depend on serialization (breaks the dependency cycle); posts Scene Loaded/Unloaded events:
  ```cpp
  #pragma once
  #include "ember/scene/Scene.h"
  #include "ember/core/Result.h"
  #include <memory>
  #include <string>
  #include <vector>

  namespace ember {
  class EventBus;

  class SceneManager {
  public:
      explicit SceneManager(EventBus& bus) : m_bus(bus) {}
      Result<void, std::string> load(const std::filesystem::path& path);          // SCM-01
      Result<void, std::string> loadAdditive(const std::filesystem::path& path);  // SCM-02
      void   unload(std::string_view name);                                       // SCM-03
      Scene& getActive();                                                         // SCM-04
      void   transition(const std::filesystem::path& path);                       // SCM-05 (deferred)
      void   endFrame();   // applies a pending transition() at end of frame
  private:
      EventBus&                            m_bus;
      std::vector<std::unique_ptr<Scene>>  m_scenes;
      Scene*                               m_active = nullptr;
      std::optional<std::filesystem::path> m_pendingTransition;
  };
  } // namespace ember
  ```
  > `load`/`loadAdditive` call into `ember_serialization` (Phase 5). Until Phase 5 lands, stub them to return an `Err("serialization not yet implemented")` so the module builds; wire them at the end of Phase 5. Post `SceneLoadedEvent`/`SceneUnloadedEvent` via `m_bus` (`SCM-01/03`).

### 4.4 Tests

- [x] `tests/scene/TestHierarchy.cpp` — `Scene_FindByNameAndTag`, `Scene_SetGetParent`, `Transform_WorldPropagation` (3-level), `Transform_DirtyOnMutation`.

**Verify Phase 4:**
```sh
cmake --build --preset debug --target ember_scene
ctest --preset debug -R "Scene_|Transform_"
```

---

## Phase 5 — Serialization (`engine/serialization/`)

Implements `SER-01 … SER-11`. yaml-cpp is included **only** in these `.cpp` files.

### 5.1 `YAMLSerializer.h` (public, YAML-free header)

- [x] Create `engine/serialization/include/ember/serialization/YAMLSerializer.h` (yaml-free; shared logic factored into internal `src/SerializationDetail.h`):
  ```cpp
  #pragma once
  #include "ember/core/Result.h"
  #include <string>
  #include <filesystem>

  namespace ember {
  class Scene;

  struct YAMLSerializer {
      static std::string serialize(const Scene& scene);                       // SER-01
      static Result<void, std::string> saveToFile(const Scene&, const std::filesystem::path&);
  };
  struct YAMLDeserializer {
      static Result<void, std::string> deserialize(const std::string& yaml, Scene& out); // SER-02
      static Result<void, std::string> loadFromFile(const std::filesystem::path&, Scene& out);
  };
  } // namespace ember
  ```

### 5.2 `YAMLSerializer.cpp` / `YAMLDeserializer.cpp`

- [x] Implement serialization (`SER-05`). Deviation: entities are emitted as a **flat list** with hierarchy carried by the `Parent`/`Children` components (not nested `children:`), which simplifies two-pass loading; format otherwise matches vision §12 (`version`, `name`, `settings`, `entities[].{id,name,components}`):
  ```yaml
  version: 1
  name: "Level 01"
  settings:
    backgroundColor: [0.1, 0.1, 0.15, 1.0]
  entities:
    - id: 1234567890
      name: "Player"
      components:
        Transform: { position: [0,0,0], rotation: 0.0, scale: [1,1,1] }
      children:
        - id: 1234567891
          ...
  ```
- [x] Drive component emission through `ComponentRegistry::all()` + each component's `reflect()` visitor; no per-component code (`SER-03`).
- [x] **Two-pass deserialize** (`SER-04`; SER-09 relaxed — relationships preserved, but EnTT assigns new numeric ids on load): pass 1 — create every entity, recording a `fileId → Entity` map (preserve the stored `id`); pass 2 — populate components, resolving `Parent`/`Children` entity references through the map.
- [x] Write a `version` field; reject unknown versions with a clear `Err` (`SER-06`).
- [x] Skip unregistered components with `EMBER_LOG_WARN`, never abort (`SER-07`).
- [x] Deterministic output (`SER-08`): components emitted in registry order; re-serializing the same scene is byte-identical.
- [x] Prefab support (`SER-10`): `serializePrefab`/`instantiatePrefab` reuse the same machinery; root sub-tree collected via `Children`.
- [~] Wire `SceneManager::load`/`loadAdditive` via the `setLoader()` hook (install `YAMLDeserializer::loadFromFile` at the app layer); `load` failure leaves the active scene intact (`SCM-06`).

### 5.3 Tests

- [x] `tests/serialization/TestSceneSerialization.cpp` — `Serialize_MultiEntityRoundtrip`, `Serialize_HierarchyRoundtrip` (3-level), `Serialize_StableOutput`, `Serialize_UnknownComponentSkipped`, `Serialize_PrefabRoundtrip`.

**Verify Phase 5:**
```sh
cmake --build --preset debug --target ember_serialization
ctest --preset debug -R "Serialize_"
```

---

## Phase 6 — Tests Wiring, Headless Demo, CI & Acceptance

### 6.1 Extend the test target

- [x] In `tests/CMakeLists.txt`, add the new sources (linked via the `ember` aggregate, which now includes the new modules):
  ```cmake
  target_sources(ember_tests PRIVATE
      events/TestEventBus.cpp
      events/TestSignal.cpp
      ecs/TestWorld.cpp
      ecs/TestScheduler.cpp
      ecs/TestReflection.cpp
      scene/TestHierarchy.cpp
      serialization/TestSceneSerialization.cpp
  )
  target_link_libraries(ember_tests PRIVATE
      ember_events ember_ecs ember_scene ember_serialization)
  ```
- [x] Keep `catch_discover_tests(ember_tests PROPERTIES SKIP_RETURN_CODE 4)` from Epic 02 (Release-skip support).

### 6.2 Headless sandbox demo (`SBX`-style, no window)

- [ ] Add a headless demo (e.g., `sandbox/ecs_demo.cpp` as a second target `ember_ecs_demo`, or guard `main.cpp` behind a flag) that:
  - creates a `World` + `Scene`, spawns 1000 entities with `Transform + Tag`,
  - builds a 3-level hierarchy, runs the `SystemScheduler` `PreUpdate` (TransformSystem) a few frames,
  - serializes to a temp `.escene`, deserializes into a fresh scene, asserts equality,
  - prints entity counts + timing. No GL, no window (matches epic note: "headless loop").
- [x] Performance smoke test `ECS_ThousandEntitiesIterate` (`tests/ecs/TestPerf.cpp`) creates + iterates 1000 entities (timing left loose for CI). This doubles as the headless ECS exercise; a standalone demo executable is deferred (optional).

### 6.3 Sanitizers & CI

- [ ] Run `cmake --preset sanitize && cmake --build --preset sanitize && ctest --preset sanitize` (or the Windows trap-mode fallback) — zero ASan/UBSan reports, clean leak report (`NFR-03`).
- [ ] Confirm header hygiene (`NFR-04`): grep that no public header under `engine/serialization/include` includes `yaml-cpp`, and that `ember_serialization`/`ember_scene` public headers don't transitively pull yaml-cpp. (EnTT in `ember_ecs` headers is accepted per the cross-cutting decision.)
  ```sh
  ! grep -rIl "yaml-cpp\|yaml.h" engine/*/include
  ```
  Verified clean: no `#include` of yaml-cpp in any public header.
- [ ] Push; confirm all three CI jobs (Windows/MSVC, Ubuntu/GCC, macOS/Clang) are green with zero warnings.

### 6.4 Acceptance checklist (from `02-requirements.md` §10)

- [ ] 1000 entities (`Transform + Tag`) created, queried, iterated within one frame budget, no spikes.
- [ ] 3-level hierarchy serializes and round-trips cleanly, IDs + parent/child links preserved.
- [ ] `EventBus` posts/dispatches `WindowResizeEvent`; subscriber gets correct data; token auto-unsubscribe works.
- [ ] `SceneManager::load` + serialization save verified against a multi-entity scene.
- [ ] All Catch2 tests pass under `debug` and `release`.
- [ ] No EnTT/yaml-cpp leak into public headers per the agreed policy (yaml-cpp fully hidden; EnTT confined to `ember_ecs` headers).
- [ ] Sanitize preset clean.
- [ ] CI green on `main`, zero warnings, all three platforms.

---

## Suggested Commit Sequence

1. `Epic 03: scaffold events/ecs/scene/serialization modules + CMake wiring` (Phase 0)
2. `Epic 03: event bus + signal with tests` (Phase 1)
3. `Epic 03: ECS World + system scheduler with tests` (Phase 2)
4. `Epic 03: reflection, component registry, built-in components` (Phase 3)
5. `Epic 03: scene, hierarchy, transform propagation, scene manager` (Phase 4)
6. `Epic 03: YAML scene/prefab serialization + round-trip tests` (Phase 5)
7. `Epic 03: headless ECS demo, perf test, CI green` (Phase 6)

Each commit should leave the build green and CI passing.
