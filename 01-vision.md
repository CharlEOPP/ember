# Ember Engine — Architecture Vision

**Version:** 0.1 — Initial Design  
**Status:** Draft  
**Audience:** Engine contributors, future maintainers

---

## 1. Project Overview

Ember is a lightweight, cross-platform game engine written in modern C++20. It is designed first for 2D games, but every architectural decision is made with 3D evolution in mind. The guiding principle is that **complexity is earned, not assumed** — subsystems start minimal and grow through well-defined extension points rather than premature abstraction.

### Design Philosophy

- **Clarity over cleverness.** Code should be readable by a competent C++ developer without engine experience.
- **Explicit over implicit.** Systems declare their dependencies. Side effects are visible.
- **Data-oriented where it matters.** Hot paths (rendering, physics, ECS queries) use cache-friendly layouts. Cold paths (editor, asset import) use idiomatic OOP.
- **No hidden magic.** Serialization, scripting, and editor integration are built on the same public API available to game code.

---

## 2. Repository & Folder Structure

```
ember/
├── CMakeLists.txt                  # Root build file
├── cmake/                          # CMake modules, toolchains, find scripts
│   ├── CompilerOptions.cmake
│   ├── Dependencies.cmake
│   └── Platform.cmake
├── engine/                         # Core engine library (libember)
│   ├── CMakeLists.txt
│   ├── core/                       # Foundation: types, logging, assert, memory
│   ├── platform/                   # OS abstraction: windowing, time, filesystem
│   ├── ecs/                        # Entity-Component-System
│   ├── events/                     # Event bus and typed event queue
│   ├── input/                      # Input abstraction
│   ├── assets/                     # Asset registry, loaders, handles
│   ├── scene/                      # Scene graph, scene manager
│   ├── renderer/                   # Renderer abstraction + 2D renderer
│   │   ├── api/                    # Backend-agnostic RHI (Render Hardware Interface)
│   │   ├── 2d/                     # Sprite, batch, text, tilemap
│   │   └── camera/                 # Camera component + projection math
│   ├── serialization/              # Reflection + YAML/binary serializers
│   ├── scripting/                  # C++ script component system
│   └── debug/                      # Profiler, debug draw, stats overlay
├── editor/                         # Ember Editor application
│   ├── CMakeLists.txt
│   ├── panels/                     # SceneHierarchy, Inspector, AssetBrowser, Viewport
│   ├── gizmos/                     # Transform gizmos
│   ├── playmode/                   # Play/pause/stop state machine
│   └── main.cpp
├── runtime/                        # Standalone game runtime (thin launcher)
│   ├── CMakeLists.txt
│   └── main.cpp
├── sandbox/                        # Developer test bed / sample game
├── tests/                          # Unit and integration tests (Catch2)
├── third_party/                    # Vendored or fetched dependencies
│   ├── glfw/
│   ├── glad/
│   ├── glm/
│   ├── entt/
│   ├── imgui/
│   ├── spdlog/
│   ├── yaml-cpp/
│   └── stb/
└── docs/                           # Architecture docs, diagrams
```

### Key Structural Rules

1. `engine/` has **zero** dependency on `editor/`. The editor depends on the engine.
2. `engine/core/` has **zero** dependency on any other engine subsystem.
3. Subsystems communicate through the event bus or explicit dependency injection — never through globals or singletons (with one exception: the logging system, which uses a thread-safe singleton for ergonomics).
4. All third-party headers are isolated behind wrapper headers in `engine/core/vendor/` so that switching libraries requires changes in exactly one place.

---

## 3. Build System

### CMake Design

The project uses **CMake 3.25+** with **FetchContent** for dependency management. Each subsystem is a proper CMake target with explicit public/private include directories and link libraries.

```cmake
# engine/ecs/CMakeLists.txt — example target definition
add_library(ember_ecs STATIC
    EntityRegistry.cpp
    ComponentStorage.cpp
    SystemScheduler.cpp
)

target_include_directories(ember_ecs
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ember_ecs
    PUBLIC  ember_core
    PRIVATE EnTT::EnTT
)

target_compile_features(ember_ecs PUBLIC cxx_std_20)
```

The root `libember` is an INTERFACE target that aggregates all subsystem targets, so game code links against a single target.

### Supported Configurations

| Config      | Use                               |
|-------------|-----------------------------------|
| Debug       | Full assertions, no optimization  |
| RelWithDbg  | Optimized + debug info (profiling)|
| Release     | Fully optimized, assertions off   |
| Sanitize    | Debug + ASan/UBSan                |

### Platform Matrix

| Platform | Compiler    | Graphics API     | Status     |
|----------|-------------|------------------|------------|
| Windows  | MSVC / Clang| OpenGL 4.6       | Primary    |
| macOS    | Clang       | OpenGL 4.1       | Supported  |
| Linux    | GCC / Clang | OpenGL 4.6       | Supported  |
| Web      | Emscripten  | WebGL 2 / OpenGL ES 3 | Future |

Metal and Vulkan backends are **explicitly deferred** but the RHI abstraction layer is designed to accommodate them.

---

## 4. Core Foundation (`engine/core/`)

### Purpose
Provide the primitive types, utilities, logging, assertions, and memory tools that every other subsystem depends on.

### Responsibilities
- Primitive type aliases (`u8`, `u32`, `f32`, `i64`, etc.)
- Assertion macros with file/line context and configurable handler
- Logger (spdlog wrapper with engine/game channels)
- Compile-time and runtime utilities (UUID generation, string hashing, bit manipulation)
- Custom allocators (arena, pool) for hot-path use
- `Result<T, E>` and `Option<T>` types wrapping `std::expected` / `std::optional`
- Platform detection macros

### Public API (representative)

```cpp
// Types
using u32 = std::uint32_t;
using f32 = float;
using UUID = std::uint64_t;

// Logging
EMBER_LOG_INFO("Loaded asset: {}", path);
EMBER_LOG_WARN("Missing texture, using fallback");
EMBER_ASSERT(ptr != nullptr, "Null pointer in {}", __func__);

// Arena allocator
ArenaAllocator arena(1024 * 1024); // 1MB
auto* obj = arena.alloc<MyStruct>();
```

### Future 3D Considerations
None — core is topology-agnostic by definition.

---

## 5. ECS Architecture (`engine/ecs/`)

### Purpose
The Entity-Component-System is the central data model. All game objects are entities; all data lives in components; all behavior lives in systems.

### Technology Choice: EnTT

We use **EnTT** as the underlying registry for its proven performance, sparse-set storage guarantees, and C++20 compatibility. We wrap it behind our own API to:
1. Provide a stable surface should we ever need to swap the backend
2. Add engine-level lifecycle hooks (OnComponentAdded, OnComponentRemoved events)
3. Enforce our naming and error-handling conventions

### Core Concepts

```
Entity   — a 64-bit identifier (index + generation)
Component — a plain data struct with no logic
System    — a function or class that queries and transforms components
World     — owns the registry, the system scheduler, and the component event hooks
```

### Responsibilities
- Create / destroy entities
- Add, remove, get, has components
- Query entities by component sets (views, groups)
- System registration and ordered execution
- Component lifecycle events (added/removed/modified)
- Entity tagging and naming (debug only)

### Public API

```cpp
World world;

// Entity creation
Entity player = world.create();
world.emplace<Transform>(player, glm::vec3{0, 0, 0});
world.emplace<SpriteRenderer>(player, "player_idle");
world.emplace<PlayerController>(player);

// Query
for (auto [entity, transform, sprite] : world.view<Transform, SpriteRenderer>()) {
    sprite.update(transform.position);
}

// Destruction
world.destroy(player);

// Lifecycle hooks
world.on_construct<RigidBody>().connect<&PhysicsSystem::onBodyAdded>();
```

### Internal Implementation

- Storage: per-component sparse sets (EnTT default)
- Archetype-style groups available for hot queries (Position + Velocity etc.)
- Systems stored in a `SystemScheduler` with explicit ordering and optional parallel execution using `std::execution::par_unseq` on independent system groups
- Component metadata registered at startup via a static `ComponentRegistry` used by the serializer and inspector

### Future 3D Considerations
- Add `MeshRenderer`, `Light`, `Collider` components — no architectural change needed
- System scheduler gains dependency graph for GPU-side work (render graph integration)

---

## 6. Event System (`engine/events/`)

### Purpose
Decoupled communication between subsystems without creating hard compile-time dependencies.

### Design: Typed Event Bus + Immediate Dispatch Queue

Two mechanisms coexist:

| Mechanism       | Use case                                          |
|-----------------|---------------------------------------------------|
| `EventBus`      | Deferred, buffered events processed each frame   |
| `Signal<T>`     | Immediate synchronous callbacks (EnTT signals)   |

```
EventBus is for: input events, scene load/unload, asset ready
Signal<T> is for: component lifecycle, tight system coupling
```

### Public API

```cpp
// Posting an event
struct WindowResizeEvent { u32 width, height; };
bus.post<WindowResizeEvent>(1920, 1080);

// Subscribing
bus.subscribe<WindowResizeEvent>([](const WindowResizeEvent& e) {
    camera.setAspect((float)e.width / e.height);
});

// Processing (called once per frame by the main loop)
bus.dispatch();
```

### Internal Implementation
- Per-type ring buffers to avoid heap allocation for common events
- Subscriber lists stored as `std::vector<std::function<>>` per event type, keyed by `std::type_index`
- Subscription handles returned as RAII tokens — subscribers auto-unregister on destruction

### Future 3D Considerations
Network events, physics contact events — same mechanism, new event types.

---

## 7. Platform & Windowing (`engine/platform/`)

### Purpose
Abstract OS-specific functionality so upper layers are platform-blind.

### Responsibilities
- Window creation, resize, close (GLFW wrapper)
- OpenGL context creation and swap-chain management
- High-resolution timer / delta-time
- Filesystem paths (using `std::filesystem`)
- Clipboard access
- Message boxes (assertion failures, fatal errors)

### Public API

```cpp
WindowSpec spec;
spec.title  = "Ember Runtime";
spec.width  = 1280;
spec.height = 720;
spec.vsync  = true;

Window window(spec);
window.setEventCallback([&bus](Event& e) { bus.post(e); });

while (!window.shouldClose()) {
    window.pollEvents();
    // ...
    window.swapBuffers();
}
```

### Future 3D Considerations
- Replace GLFW with SDL3 if we need controller rumble, mobile, or console support
- Add `Surface` / `Swapchain` abstractions when adding Vulkan

---

## 8. Input System (`engine/input/`)

### Purpose
Translate raw platform input events into a clean, polling-friendly API.

### Responsibilities
- Keyboard state (pressed, held, released)
- Mouse state (position, delta, buttons, scroll)
- Gamepad axes and buttons
- Input action mapping (named actions bound to physical keys)
- Input context stack (game vs. editor vs. UI)

### Public API

```cpp
// Polling (preferred in game systems)
if (Input::isKeyDown(Key::Space)) { jump(); }
if (Input::isMouseButtonPressed(Mouse::Left)) { fire(); }
glm::vec2 mouseDelta = Input::getMouseDelta();

// Action map
InputAction moveLeft("move_left");
moveLeft.bindKey(Key::A);
moveLeft.bindKey(Key::Left);
moveLeft.bindAxis(Gamepad::LeftStickX, -1.0f);
```

### Internal Implementation
- State tables updated by the platform window's event callback each frame
- Previous-frame state cached to compute pressed/released edges
- Action map queries resolved through a `std::unordered_map<StringHash, ActionDef>`

---

## 9. Asset Management (`engine/assets/`)

### Purpose
Load, cache, and lifetime-manage all game resources.

### Design: Handle-Based Asset System

Assets are accessed through typed handles — never raw pointers. The `AssetManager` owns all loaded data. Handles are reference-counted; an asset is evicted when its ref count reaches zero (unless pinned).

```
AssetHandle<Texture2D>  — thin 64-bit ID
AssetManager            — registry of all handles → asset data
AssetLoader<T>          — interface for type-specific loading
AssetDatabase           — maps virtual paths to physical paths + metadata
```

### Public API

```cpp
// Synchronous load (editor / startup)
auto tex = assets.load<Texture2D>("textures/player.png");
tex->bind(0);

// Async load (runtime)
auto future = assets.loadAsync<AudioClip>("sfx/jump.wav");
future.then([](AssetHandle<AudioClip> clip) { audio.play(clip); });

// Unload happens automatically when handle goes out of scope
```

### Internal Implementation
- Asset paths use virtual path strings (e.g., `"textures/player.png"`) resolved through the `AssetDatabase` to real filesystem paths
- Each asset type registers a loader via `AssetManager::registerLoader<T, Loader>()`
- Hot-reload: file watcher monitors asset directories; changed files trigger loader re-execution and handle value swap (transparent to holders)
- Asset metadata stored as sidecar `.meta` YAML files

### Asset Types (initial)
| Type         | Loader          | Format          |
|--------------|-----------------|-----------------|
| Texture2D    | STB Image       | PNG, JPG, BMP   |
| AudioClip    | miniaudio       | WAV, OGG        |
| Font         | stb_truetype    | TTF             |
| Shader       | GLSL compiler   | .glsl           |
| Scene        | YAML deserializer| .escene        |
| Tilemap      | Custom parser   | .etilemap       |

### Future 3D Considerations
- Add `Mesh`, `Material`, `AnimationClip`, `Cubemap`
- Async streaming becomes critical — extend `loadAsync` with priority queues and streaming chunks

---

## 10. Scene Management (`engine/scene/`)

### Purpose
Organize entities into scenes; manage scene transitions and lifecycle.

### Design

A `Scene` is a named container that owns a `World` (ECS registry) and a root entity hierarchy metadata table. The `SceneManager` controls which scenes are active.

```
SceneManager
  └── Scene[]
        ├── World (ECS registry)
        ├── EntityHierarchy (parent-child transform tree)
        └── SceneSettings (gravity, ambient light, etc.)
```

### Responsibilities
- Load / unload / transition between scenes
- Additive scene loading (UI scene on top of game scene)
- Entity parent-child hierarchy (transform propagation)
- Scene serialization / deserialization

### Public API

```cpp
// Loading
sceneManager.load("scenes/level_01.escene");
sceneManager.loadAdditive("scenes/hud.escene");

// Access
Scene& active = sceneManager.getActive();
Entity camera = active.findByName("MainCamera");

// Instantiation
Entity enemy = active.instantiate(prefabHandle, glm::vec3{100, 0, 0});

// Hierarchy
active.setParent(weapon, player);
glm::mat4 worldMatrix = active.getWorldTransform(weapon);
```

### Internal Implementation
- Parent-child stored as `Parent` and `Children` components on entities
- World transform computed lazily and cached in a `WorldTransform` component; dirty flag set by `Transform` mutation
- Scene file format described in §14

---

## 11. Renderer (`engine/renderer/`)

### Purpose
Draw things on screen efficiently, with a clear separation between the rendering API abstraction, the 2D renderer, and future 3D renderer.

### Architecture: Three Layers

```
┌─────────────────────────────────────────┐
│           Game / Editor Code            │
├─────────────────────────────────────────┤
│    Renderer2D  (high-level draw calls)  │
├─────────────────────────────────────────┤
│  RHI — Render Hardware Interface        │
│  (VertexBuffer, Shader, Framebuffer...) │
├─────────────────────────────────────────┤
│    OpenGL Backend (initial)             │
│    (Vulkan / Metal — future)            │
└─────────────────────────────────────────┘
```

### 11.1 RHI (`engine/renderer/api/`)

#### Purpose
Backend-agnostic abstraction over GPU primitives.

#### Public API

```cpp
// Resources (all returned as shared_ptr for simplicity at this stage)
auto vbo    = RHI::createVertexBuffer(vertices, usage);
auto shader = RHI::createShader("sprite.glsl");
auto tex    = RHI::createTexture2D(spec);
auto fbo    = RHI::createFramebuffer(spec);

// Commands
RHI::setViewport(0, 0, 1280, 720);
RHI::bindShader(shader);
RHI::bindTexture(tex, 0);
RHI::drawIndexed(vbo, ibo, count);
```

#### Internal Implementation
- All RHI types are abstract base classes; OpenGL implementations live in `renderer/api/opengl/`
- Factory function `RHI::create()` selects backend at startup based on platform/config
- No virtual dispatch in hot inner loops — draw calls are batched and submitted through a `CommandBuffer` that is replayed in one pass

### 11.2 Renderer2D (`engine/renderer/2d/`)

#### Purpose
High-level API for drawing sprites, quads, text, and tilemaps efficiently.

#### Responsibilities
- Sprite batching (texture atlases, sorted draw calls)
- Text rendering (bitmap fonts via stb_truetype)
- Line/shape debug draw
- Tilemap rendering
- Particle system rendering (CPU-driven initially)

#### Public API

```cpp
Renderer2D::beginScene(camera);

Renderer2D::drawSprite(transform, sprite);
Renderer2D::drawQuad({100, 200}, {64, 64}, Color::Red);
Renderer2D::drawText("Hello World", font, {10, 10}, 24.0f);
Renderer2D::drawLine({0,0}, {100,100}, Color::White);

Renderer2D::endScene();  // flushes all batches
```

#### Internal Implementation
- **Sprite batcher:** quads accumulated in a mapped vertex buffer, flushed per texture atlas or when buffer is full (4096 quads default)
- **Draw call sorting:** opaque front-to-back, transparent back-to-front, grouped by texture to minimize binds
- **Vertex layout:** `[vec2 pos, vec2 uv, vec4 color, float texIndex]`
- Batching is stateless per-frame — no retained render graph overhead

### 11.3 Camera System (`engine/renderer/camera/`)

```cpp
struct CameraComponent {
    ProjectionType type;   // Orthographic | Perspective
    float size;            // ortho half-height OR fov degrees
    float near, far;
    bool  isPrimary;
};
```

The `CameraSystem` queries entities with `CameraComponent` + `Transform`, computes view/projection matrices, and exposes them to the renderer. Orthographic projection is default for 2D; the struct is identical for 3D — only `type` changes.

#### Future 3D Considerations
- Add `Renderer3D` as a sibling to `Renderer2D`, sharing the same RHI layer
- Render graph replaces the simple flush model when multi-pass rendering (shadows, post-processing) is needed
- Material system attaches to RHI shaders with a uniform buffer / descriptor set abstraction

---

## 12. Serialization (`engine/serialization/`)

### Purpose
Save and load engine state (scenes, assets, editor layout) in a human-readable format.

### Format: YAML for scenes, binary for runtime

| Format  | Use                            | Library     |
|---------|--------------------------------|-------------|
| YAML    | Scenes, prefabs, settings      | yaml-cpp    |
| Binary  | Compiled/shipped asset packs   | Custom      |

### Design: Reflection-Driven

Rather than hand-writing serialize/deserialize for every component, we use a lightweight **compile-time reflection** system to enumerate struct fields.

```cpp
// Component registration (once, in a .cpp file)
EMBER_REFLECT(Transform) {
    EMBER_FIELD(position)
    EMBER_FIELD(rotation)
    EMBER_FIELD(scale)
}

EMBER_REFLECT(SpriteRenderer) {
    EMBER_FIELD(texturePath)
    EMBER_FIELD(color)
    EMBER_FIELD(layer)
}
```

The `EMBER_REFLECT` macro registers the component with the `ComponentRegistry`, which the `YAMLSerializer` queries at scene save/load time. No additional serialization code needed per component.

### Scene File Format

```yaml
# level_01.escene
version: 1
name: "Level 01"
settings:
  backgroundColor: [0.1, 0.1, 0.15, 1.0]

entities:
  - id: 1234567890
    name: "Player"
    components:
      Transform:
        position: [0.0, 0.0, 0.0]
        rotation: 0.0
        scale: [1.0, 1.0]
      SpriteRenderer:
        texture: "textures/player_idle.png"
        color: [1.0, 1.0, 1.0, 1.0]
        layer: 5
      PlayerController:
        speed: 200.0
        jumpForce: 400.0
    children:
      - id: 1234567891
        name: "Sword"
        components: { ... }
```

### Future 3D Considerations
Reflection system is dimension-agnostic; new 3D components register the same way. Binary format path becomes the primary shipping format with the YAML format remaining for authoring.

---

## 13. Scripting (`engine/scripting/`)

### Purpose
Allow game logic to be written in C++ with an ergonomic, Unity-MonoBehaviour-style API, attached to entities as components.

### Design

Scripts are C++ classes that inherit `ScriptComponent` and override lifecycle methods. They are attached to entities as first-class ECS components. The `ScriptSystem` queries all `ScriptComponent` entities each frame.

```cpp
class PlayerController : public ScriptComponent {
public:
    float speed     = 200.0f;
    float jumpForce = 400.0f;

    void onStart() override {
        rb = getComponent<RigidBody>();
    }

    void onUpdate(f32 dt) override {
        float h = Input::getAxis("Horizontal");
        rb->applyForce({h * speed, 0});

        if (Input::isKeyPressed(Key::Space)) {
            rb->applyImpulse({0, jumpForce});
        }
    }

    void onDestroy() override { }

private:
    RigidBody* rb = nullptr;
};

// Attachment (in editor or scene file)
world.emplace<PlayerController>(player);
```

### ScriptComponent API Surface

```cpp
class ScriptComponent {
protected:
    // Lifecycle
    virtual void onStart()           {}
    virtual void onUpdate(f32 dt)    {}
    virtual void onFixedUpdate(f32 dt){}
    virtual void onDestroy()         {}

    // Collision callbacks
    virtual void onCollisionEnter(Entity other) {}
    virtual void onCollisionExit(Entity other)  {}

    // Helpers
    template<typename T> T*   getComponent();
    template<typename T> T*   addComponent();
    template<typename T> void removeComponent();
    Entity                    getEntity() const;
    Scene&                    getScene()  const;
    void                      destroy();
};
```

### Hot Reload Plan (Future)

Scripts are compiled into a separate shared library (`game_scripts.dll/.so`). At runtime:
1. File watcher detects source change
2. Build system recompiles the scripts library
3. Engine pauses, serializes all script component state, unloads old library, loads new library, deserializes state, resumes

This requires script state to be serializable — the same reflection system from §12 handles this.

---

## 14. Editor Architecture (`editor/`)

### Purpose
A full in-process editor built on Dear ImGui, running the engine's full runtime inside a viewport.

### Architecture: Editor is a Client of the Engine

The editor never bypasses the engine API. Everything it does — creating entities, modifying components, managing assets — uses the exact same API available to game code. This guarantees parity and prevents editor-only bugs.

### Editor State Machine

```
┌─────────┐   Play    ┌──────────┐   Pause   ┌──────────────┐
│  EDIT   │ ────────► │ PLAYING  │ ─────────► │   PAUSED     │
│  mode   │ ◄──────── │          │ ◄───────── │              │
└─────────┘   Stop    └──────────┘   Resume   └──────────────┘
```

In **Edit mode:** the engine's Update loop is suspended; only rendering and editor panels run.  
In **Play mode:** a deep copy of the scene is made; the live copy runs; on Stop, the copy is discarded and the original is restored.

### Panels

#### Scene Hierarchy Panel
- Displays entity tree using ImGui's `TreeNode`
- Drag-to-reparent, right-click context menu (create, delete, duplicate)
- Selection drives the Inspector

#### Inspector Panel
- Iterates registered `ComponentRegistry` entries for the selected entity
- Each component type has an optional custom `drawUI(ComponentType&)` override; default is auto-generated from reflection data
- Undo/redo stack: every inspector mutation posts a `Command` to the `CommandHistory`

#### Asset Browser Panel
- Flat/tree view of `assets/` directory
- Thumbnail generation for textures
- Double-click opens asset in context-appropriate tool
- Drag-and-drop from browser to Inspector fields

#### Viewport Panel
- Scene rendered to a `Framebuffer`; texture displayed in an ImGui image
- Mouse/keyboard events forwarded to the game input system only in Play mode
- Camera is an editor-only free-camera in Edit mode

#### Gizmos
- Transform gizmo (translate/rotate/scale) implemented with ImGuizmo
- Overlay rendering via `DebugDraw` for colliders, camera frustums, etc.

### Undo/Redo

```cpp
struct Command {
    virtual void execute() = 0;
    virtual void undo()    = 0;
};

class CommandHistory {
    void push(std::unique_ptr<Command> cmd);
    void undo();
    void redo();
};
```

Every editor action that modifies scene state is wrapped in a `Command`. Inspector field edits, entity creation/deletion, reparenting — all reversible.

---

## 15. Dependency Graph

```
core
 └── platform
      ├── events
      │    └── input
      ├── assets
      │    └── scene
      │         ├── ecs
      │         ├── renderer (RHI → 2D)
      │         ├── scripting
      │         └── serialization
      └── debug

editor → (all engine subsystems)
runtime → scene, renderer, scripting, input
```

**Hard rules:**
- Arrows point from dependent → dependency only
- No cycles
- `ecs` does not depend on `renderer` or `scene`
- `renderer` does not depend on `ecs` directly; the `RenderSystem` in `scene` bridges them

---

## 16. Memory Strategy

| Context              | Strategy                         |
|----------------------|----------------------------------|
| ECS component data   | EnTT sparse sets (pooled)        |
| Frame-lifetime data  | Per-frame linear allocator, reset each frame |
| Asset data           | Ref-counted heap via `AssetManager` |
| Render vertex data   | GPU buffer mapped persistently    |
| Editor / tooling     | Standard `std::` allocators       |
| String interning     | Global string table (hashed)      |

We do **not** write a general-purpose allocator. We use targeted allocators for measurably hot paths only.

---

## 17. Logging & Debugging

### Logging
spdlog with two logger channels:
- `ember` — engine-internal messages
- `game`  — game/script messages

Log levels: `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`  
`FATAL` triggers a breakpoint in debug builds and calls `std::terminate` in release.

### Debug Draw
`DebugDraw::line()`, `DebugDraw::box()`, `DebugDraw::circle()` — rendered via the 2D renderer as an overlay, compiled out in Release builds.

### Profiler
Lightweight scope-based profiler using `__FUNCTION__` macros:
```cpp
EMBER_PROFILE_SCOPE("PhysicsSystem::update");
```
Results displayed in a profiler panel in the editor (Optick integration optional).

---

## 18. Implementation Roadmap

### Milestone 0 — Foundation (2–3 weeks)
- [ ] CMake project skeleton with all third-party deps via FetchContent
- [ ] `core/` types, logging, assert
- [ ] Platform window (GLFW) + OpenGL context
- [ ] Triangle on screen via RHI
- [ ] CI pipeline (GitHub Actions, builds on Windows + Linux + macOS)

### Milestone 1 — ECS + Scene (2–3 weeks)
- [ ] EnTT integration, World wrapper
- [ ] Transform + hierarchy components
- [ ] Event bus
- [ ] Scene load/save (YAML, hard-coded component set)
- [ ] Basic serialization reflection macros

### Milestone 2 — Renderer 2D (3–4 weeks)
- [ ] Texture2D loading (STB)
- [ ] Sprite batching renderer
- [ ] Orthographic camera
- [ ] Debug draw overlay
- [ ] Text rendering (bitmap font)

### Milestone 3 — Input + Assets (1–2 weeks)
- [ ] Keyboard / mouse input system
- [ ] Asset manager + handle system
- [ ] Async asset loading

### Milestone 4 — Scripting (2 weeks)
- [ ] `ScriptComponent` base class
- [ ] `ScriptSystem` lifecycle dispatch
- [ ] Sample `PlayerController` in sandbox

### Milestone 5 — Editor v1 (4–5 weeks)
- [ ] ImGui integration, dockspace layout
- [ ] Viewport (render to framebuffer)
- [ ] Scene Hierarchy panel
- [ ] Inspector panel (reflection-driven)
- [ ] Asset Browser panel
- [ ] Play / Pause / Stop state machine
- [ ] Scene save/load from editor

### Milestone 6 — Editor v2 + Polish (3 weeks)
- [ ] Transform gizmos (ImGuizmo)
- [ ] Undo/redo system
- [ ] Tilemap component + renderer
- [ ] Profiler panel
- [ ] Prefab system

### Milestone 7 — 3D Foundation (future)
- [ ] Vulkan RHI backend
- [ ] Mesh + Material asset types
- [ ] 3D camera (perspective projection)
- [ ] Forward renderer (lit meshes)
- [ ] `Renderer3D` alongside `Renderer2D`

### Milestone 8 — 3D Features (future)
- [ ] PBR material system
- [ ] Shadow maps
- [ ] Physics integration (Jolt Physics)
- [ ] Skeletal animation
- [ ] Post-processing stack

### Milestone 9 — Scripting Hot Reload + Networking (future)
- [ ] Shared library hot reload for scripts
- [ ] ENet / GameNetworkingSockets integration
- [ ] Replicated component system

---

## 19. Open Source & Code Quality

- **License:** MIT
- **Formatting:** `clang-format` enforced by CI (Google style, 4-space indent)
- **Static analysis:** `clang-tidy` with a curated `.clang-tidy` config (no modernize-use-trailing-return unless it genuinely improves clarity)
- **Tests:** Catch2, targeting >80% coverage on ECS, serialization, and math utilities
- **Documentation:** Doxygen on public API; architecture docs in `docs/` as Markdown
- **PR policy:** All PRs require at least one passing CI run and one review

---

*This document is a living specification. Sections marked "Future" define the intended extension path but are not commitments. Architecture decisions should be revisited when a milestone is started, not assumed to be final from this document alone.*
