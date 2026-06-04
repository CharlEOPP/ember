# Epic 05 — Input & Asset Management: Implementation Plan

**Status:** Not started  
**Document:** 03-implementation-plan.md  
**Refs:** epic.md, 02-requirements.md, 01-vision.md §8/§9

Work through the phases in order; each ends in a buildable, testable state. Don't proceed until the current phase's boxes are ticked and its verification passes. Requirement IDs (e.g. `AM-05`) refer to `02-requirements.md`.

**Grounding (current repo state):**

| Fact | Consequence for this epic |
|------|---------------------------|
| No `engine/input` or `engine/assets` yet | both are new modules + targets |
| `Window` exposes `getNativeHandle() → GLFWwindow*` | `InputManager` installs GLFW callbacks itself; **platform stays decoupled from input** |
| `Window::setEventCallback` already used (resize) | input uses the native handle, not that single slot |
| EventBus + `Events.h` plain structs exist (Epic 03) | input re-broadcasts `KeyPressedEvent` etc.; reload uses `AssetReloadedEvent` |
| yaml-cpp, stb (`STB_INCLUDE_DIR`), glm, EnTT, Catch2 fetched | reuse; **add miniaudio** |
| `SpriteRenderer.texture` is `shared_ptr<ITexture2D>` + `texturePath` | migrate to `AssetHandle<Texture2D>` (Phase 7) |
| Texture/font loaders live in `engine/renderer/2d/loaders/` | **relocate** into `engine/assets/loaders/` (Phase 4) |

**Cross-cutting decisions (confirm before starting):**

- **GLFW translation lives in `InputManager.cpp`** (not platform). `engine/input` links `glfw` PRIVATE and installs callbacks on `window.getNativeHandle()`; `Keys.h` is GLFW-free, so no GLFW type leaks into any public header. Because GLFW callbacks are C function pointers, the manager routes them through a `static InputManager* s_active` (set in `init`).
- **Dependency direction:** `engine/assets` → `ember_renderer_api` (for `ITexture2D`), `ember_platform` (FileSystem), `ember_serialization` (scene loader), `ember_events` (reload events). `engine/renderer/2d` → `engine/assets` (SpriteRenderer holds a handle). No cycle (`renderer_api` doesn't depend on `assets`).
- **GL uploads are main-thread only.** Async loaders read bytes/pixels on a worker; the GL texture is created on the main thread via a `PendingUpload` queue drained each frame. `get<T>` returns `nullptr` until the upload lands.
- **`FileWatcher` is polling-based** (mtime scan on a background thread) for portability — far simpler than `inotify`/`ReadDirectoryChangesW`/`kqueue`, works on all three platforms, and is gated by `EMBER_HOT_RELOAD`. Native APIs are a future optimization.
- **miniaudio** is added like stb (single header, populate + `MINIAUDIO_INCLUDE_DIR`); only the `AudioClip` loader uses it. `AudioSystem` is a stub.

---

## Phase 0 — Scaffolding, miniaudio, CMake Wiring

- [ ] Create directory trees:
  ```
  engine/input/include/ember/input/    engine/input/src/
  engine/assets/include/ember/assets/  engine/assets/src/   engine/assets/loaders/
  settings/                            tests/input/   tests/assets/
  ```
- [ ] Add **miniaudio** to `cmake/Dependencies.cmake` (mirror the stb pattern):
  ```cmake
  FetchContent_Declare(miniaudio
      GIT_REPOSITORY https://github.com/mackron/miniaudio.git
      GIT_TAG        0.11.21)        # pin a release tag
  FetchContent_MakeAvailable(miniaudio)   # header-only; no CMake targets it builds
  set(MINIAUDIO_INCLUDE_DIR "${miniaudio_SOURCE_DIR}" CACHE PATH "miniaudio include dir" FORCE)
  ```
- [x] `engine/input/CMakeLists.txt`: *(done — also includes `src/InputWindowBridge.cpp`; see note below)*
  ```cmake
  add_library(ember_input STATIC src/InputManager.cpp)
  target_include_directories(ember_input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(ember_input PUBLIC ember_core ember_events glm::glm PRIVATE glfw)
  target_compile_features(ember_input PUBLIC cxx_std_20)
  ember_set_compiler_options(ember_input)
  ```
- [ ] `engine/assets/CMakeLists.txt`:
  ```cmake
  add_library(ember_assets STATIC
      src/AssetManager.cpp
      src/AssetDatabase.cpp
      src/FileWatcher.cpp
      loaders/Texture2DLoader.cpp     # relocated in Phase 4
      loaders/FontLoader.cpp
      loaders/AudioClipLoader.cpp
      loaders/stb_impl.cpp            # moved from renderer (single stb TU)
  )
  target_include_directories(ember_assets
      PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
      PRIVATE ${STB_INCLUDE_DIR} ${MINIAUDIO_INCLUDE_DIR})
  target_link_libraries(ember_assets
      PUBLIC  ember_core ember_platform ember_events ember_renderer_api ember_scene
      PRIVATE yaml-cpp::yaml-cpp ember_serialization)
  target_compile_definitions(ember_assets PRIVATE $<$<CONFIG:Debug>:EMBER_HOT_RELOAD>)
  target_compile_features(ember_assets PUBLIC cxx_std_20)
  ember_set_compiler_options(ember_assets)
  # third-party single-headers: warnings off (like glad/stb in Epic 04)
  if(MSVC)
      set_source_files_properties(loaders/stb_impl.cpp PROPERTIES COMPILE_OPTIONS "/w")
  else()
      set_source_files_properties(loaders/stb_impl.cpp PROPERTIES COMPILE_OPTIONS "-w")
  endif()
  ```
- [~] Root `CMakeLists.txt`: `add_subdirectory(engine/input)` + `ember_input` in the `ember` aggregate **done**; `engine/assets` wiring pending (Phase 3).
- [ ] Copy `settings/` next to the sandbox exe (extend the Epic 04 `copy_directory` step, or add one for `settings/`).

> **Note (input scaffold done):** `engine/input/` created and wired. A latent name clash surfaced — `platform/Event.h` (Epic 02 polymorphic events) and `events/Events.h` (Epic 03 EventBus PODs) both defined `ember::WindowResize/CloseEvent`. **Fixed** by moving the platform hierarchy into `namespace ember::platform` (touched `Event.h`, `Window.h`, `Window.cpp`, `sandbox/main.cpp`), so `init(Window&)` now lives directly in `src/InputManager.cpp` (no bridge TU). **miniaudio + `engine/assets/` scaffold are deferred to Phases 3-4** (only needed there).

**Verify Phase 0 (input):** input module configures and compiles (verified via `-fsyntax-only` against glfw/yaml/glm/spdlog). Full `cmake --preset debug` reconfigure pending the assets target.

---

## Phase 1 — Input: enums, polling, manager (`engine/input/`)

Implements `INP-01 … INP-10`.

- [x] `include/ember/input/Keys.h` — GLFW-free `enum class Key` (letters, digits, F1–F12, modifiers, arrows, space, enter, escape, tab, backspace…) and `enum class Mouse { Left, Right, Middle, Button4, Button5 }`. Explicit values chosen to match GLFW keycodes exactly, so translation is a bounds-checked cast (INP-01/02/03).
- [x] `include/ember/input/Input.h` — static facade:
  ```cpp
  namespace ember::Input {
    bool isKeyDown(Key);  bool isKeyPressed(Key);  bool isKeyReleased(Key);
    bool isMouseButtonDown(Mouse); bool isMouseButtonPressed(Mouse); bool isMouseButtonReleased(Mouse);
    glm::vec2 getMousePosition(); glm::vec2 getMouseDelta(); f32 getMouseScroll();
  }
  ```
  All forward to the active `InputManager` (null-safe ⇒ "not pressed") (INP-04..08).
- [x] `include/ember/input/InputManager.h` — owns per-frame state:
  ```cpp
  class InputManager {
  public:
      void init(Window& window);     // installs GLFW callbacks on window.getNativeHandle()
      void shutdown();
      void update();                 // roll current->previous; recompute edges; poll gamepad
      // ...querying used by the Input facade...
  private:
      static InputManager* s_active; // GLFW C callbacks route here
      std::array<bool, KeyCount>  m_curKeys{}, m_prevKeys{};
      std::array<bool, 5>         m_curMouse{}, m_prevMouse{};
      glm::vec2 m_mousePos{}, m_prevMousePos{}, m_mouseDelta{}; f32 m_scroll = 0;
      EventBus* m_bus = nullptr;
  };
  ```
- [x] `src/InputManager.cpp` (the only TU that includes `<GLFW/glfw3.h>`):
  - `initWithHandle`: store `s_active = this`; install `glfwSetKeyCallback / MouseButtonCallback / CursorPosCallback / ScrollCallback` on the native handle; translate GLFW codes → `Key`/`Mouse` (INP-03). (Window owns the GLFW user pointer, so callbacks route through `s_active`.)
  - callbacks call the `feed*` seam and `m_bus->post<KeyPressedEvent>(...)` etc. when a bus is supplied (INP-09).
  - `update`: recompute `m_pressedKeys`/`m_releasedKeys` edge arrays vs the previous snapshot, then `m_prevKeys = m_curKeys`; mouse delta; publish+reset per-frame scroll; poll gamepad (INP-07/10).
  - Edges stored per-frame (not derived on query) so results are correct regardless of poll/update order.

- [x] Tests `tests/input/TestInputState.cpp` (headless): `makeActive()` + `feedKey/feedMouse*/feedScroll` seam drives a synthetic down→hold→up over `update()` calls verifying `pressed`/`held`/`released` (INP-04/07) and mouse delta/scroll reset (INP-06).

**Verify Phase 1:** ✅ all input TUs + test compile clean (`-Wall -Wextra`, zero warnings) via staged syntax check. Runtime `ctest` pending the assets target so the full `ember_tests` exe links.

---

## Phase 2 — Action Map, Context Stack, Gamepad

Implements `ACT-01 … ACT-05`, `CTX-01 … CTX-03`, `GP-01 … GP-04`.

- [x] `include/ember/input/InputAction.h` — `InputAction` (`bindKey`, `bindMouseButton`, `bindGamepadButton`, `bindAxis(GamepadAxis, scale)`, fluent); `ActionState { pressed, held, released, axisValue }`. (Actions stored per-context in `InputManager` as `map<ctx, map<name, InputAction>>` rather than a free `InputMap`.)
- [x] `enum class GamepadAxis` / `enum class GamepadButton` (GLFW-free) in `Keys.h`.
- [x] `InputManager::getAction(name) → ActionState` aggregates all bindings (any down ⇒ held; pressed/released from key/mouse edges; axis = max magnitude) (ACT-03).
- [x] Context stack: `pushContext(name)`/`popContext()`/`activeContext()`; only the **top** context's actions resolve; raw polling unaffected (CTX-01..03).
- [x] YAML: `loadActionMap/saveActionMap` via yaml-cpp (linked `PRIVATE` to `ember_input`); `loadDefaultActionMap()` for defaults; malformed/missing ⇒ warn + defaults (ACT-04/05). Default map shipped as `settings/input.yaml`.
- [x] Gamepad: `update()` polls `glfwGetGamepadState(GLFW_JOYSTICK_1, …)`; dead-zone applied; `gamepadAxis`/`isGamepadButtonDown`; absent gamepad ⇒ neutral (GP-01..04).
- [x] Tests: `ActionMap_AggregatesBindings`, `Context_TopOnlyFires` written. *(YAML round-trip test deferred to the Phase 8 test-wiring pass — load/save code is implemented & compiles.)*

> Note: yaml-cpp linked `PRIVATE` to `ember_input` for action-map I/O (stays out of public headers). ✅

**Verify Phase 2:** ✅ compiles clean; aggregation + context tests authored. Runtime pass pending the test exe link (Phase 8).

---

## Phase 3 — Asset Core: handles, manager, database (`engine/assets/`)

Implements `AM-01 … AM-10`, `ADB-01 … ADB-04`, `LOAD-01`.

- [x] `include/ember/assets/AssetHandle.h` — `template<typename T> struct AssetHandle { u64 id = 0; static null(); valid(); operator==/!=; explicit operator bool; }` + `std::hash` specialization (AM-01..03).
- [x] `include/ember/assets/IAssetLoader.h` — typed interface + `AssetSettings` (with `get`/`getBool`):
  ```cpp
  struct AssetSettings { /* parsed from .meta */ std::unordered_map<std::string,std::string> kv; };
  template<typename T> struct IAssetLoader {
      virtual ~IAssetLoader() = default;
      virtual std::shared_ptr<T> load(const std::filesystem::path&, const AssetSettings&) = 0;
      virtual void unload(T&) {}
  };
  ```
- [x] `include/ember/assets/AssetDatabase.h` + `src/AssetDatabase.cpp` — virtual-path↔file map; `.meta` YAML read/write (uuid/type/importer/settings) via static `readMeta`/`writeMeta`; `scan(root)` creates `.meta` (fresh UUID via Epic 02 `generateUUID`) for new files; `getPath(UUID)`/`getRealPath`/`getMeta` (ADB-01..04).
- [x] `include/ember/assets/AssetManager.h` + `src/AssetManager.cpp`:
  - registry of loaders by `std::type_index` → type-erased `IAssetLoader<T>`; `registerLoader<T>(loader)`.
  - cache keyed by `(typeName, virtualPath)` → `Entry{ type, path, refcount, pinned, shared_ptr<void> data, unloadFn, reloadFn }`.
  - `load<T>` (sync): cached ⇒ bump refcount + return handle; else resolve path/settings via DB, run loader, store, return new handle (AM-04/05).
  - `get<T>` (type-checked), `release`, `pin`/`unpin`, `reload`, `refCount`, `liveCount` (AM-07..10). yaml-cpp stays in `.cpp`.
- [x] Tests (mock loaders, CPU-only): handle null/equality, caches-by-path (loader call count == 1), refcount-evicts (unload called), pin-prevents-eviction, reload re-runs loader, missing-loader ⇒ null, `.meta` YAML round-trip — in `tests/assets/TestAssetManager.cpp`.

**Verify Phase 3:** ✅ `-Werror` clean; **asset-core logic verified at runtime** via a Linux-native driver (cache/refcount/pin/reload/missing-loader all pass). Full Catch2 `ctest` deferred to the user's real build (the sandbox's prebuilt Catch2/spdlog/yaml libs are MinGW/Windows objects and can't link with Linux GCC).

---

## Phase 4 — Loaders + relocation

Implements `LOAD-02/03`, the `Texture2D` asset type.

- [x] Define the `Texture2D` asset type (`include/ember/assets/Texture2D.h`): wrapper owning `std::shared_ptr<ITexture2D>` + dimensions + `rhi()` accessor (MIG-02). Also added `Shader` wrapper (`Shader.h`).
- [x] **Relocate** `Texture2DLoader` and `FontAsset` from `engine/renderer/2d/loaders/` into `engine/assets/` as `IAssetLoader<Texture2D>` and a new `FontLoader : IAssetLoader<FontAsset>`; moved `stb_impl.cpp` too (LOAD-03). Removed from `ember_renderer_2d`'s sources; old headers deleted. `Renderer2D.cpp` now includes `ember/assets/FontAsset.h`. *(Kept the class name `FontAsset` rather than renaming to `Font`, to keep `Renderer2D::drawText`'s API stable — lower-risk than the plan's rename.)*
- [x] Add `ShaderLoader` (`IAssetLoader<Shader>`, wraps `ShaderLibrary::splitSource` + `RHI::createShader`). *(`YAMLSceneLoader` skipped: scenes already load via `SceneManager::setLoader` from `installSceneSerialization`; routing Scene through the refcounted AssetManager would be redundant.)*
- [~] `AudioClipLoader` (miniaudio) + `AudioClip` + `AudioSystem` stub — **deferred**: miniaudio isn't fetched in this environment and audio is an explicit Epic-05 stretch goal. Tracked for a follow-up (add the dep, then the loader).
- [x] `installDefaultLoaders(AssetManager&)` registers Texture2D / FontAsset / Shader loaders (LOAD-02).

**Verify Phase 4:** ✅ all relocated loaders + `installDefaultLoaders` compile clean (`-Werror`); `Renderer2D.cpp` compiles against the relocated `FontAsset` header. CMake DAG: `renderer_2d → assets → renderer_api` (no cycle). GL-path runtime + full configure verified by the user's real build.

---

## Phase 5 — Async loading + PendingUpload

Implements `AM-06/11`, `LOAD-04`, `NFR-01/02`.

- [x] Added `ThreadPool` (`include/ember/assets/ThreadPool.h`) to `AssetManager`; the cache is guarded by a `mutex` (NFR-02). Loaders gained an optional two-phase async API (`supportsAsync`/`loadCPU`/`finalize`) on `IAssetLoader<T>`.
- [x] `loadAsync<T>` returns `std::future<AssetHandle<T>>`; the worker runs `loadCPU` (decode), then enqueues a `PendingUpload` capturing the `finalize` (GPU) step. Non-async loaders fall back to a synchronous load.
- [x] `AssetManager::processPendingUploads()` runs the queued finalize steps on the main thread; `get<T>` returns `nullptr` until resident (AM-11). `Texture2DLoader` implements the split (stb decode on worker → `RHI::createTexture2D` on main).
- [~] Sandbox `processPendingUploads()` each frame — folded into the deferred sandbox demo (Phase 7).

**Verify Phase 5:** ✅ runtime driver confirms the future resolves **only after** `processPendingUploads` (loadCPU once, finalize once, `get` then valid); compiles `-Werror`. GPU upload path is the user's build.

---

## Phase 6 — Hot Reload (`FileWatcher`)

Implements `HR-01 … HR-04`.

- [x] `include/ember/assets/FileWatcher.h` + `src/FileWatcher.cpp` — polling watcher: background thread snapshots mtimes every ~250ms (configurable), reports changed paths via a thread-safe queue drained by `poll()`. Guarded by `EMBER_HOT_RELOAD` (`watch()` no-op / `poll()` empty when undefined) (HR-01/04).
- [x] `AssetManager::reloadByRealPath()` reloads every resident asset whose source resolves to a changed path; the `pollAssetHotReload()` helper (`HotReload.h/.cpp`) drains the watcher and drives it each frame (HR-02).
- [x] After reload, `pollAssetHotReload` posts `AssetReloadedEvent` (added to `engine/events/Events.h`) when a bus is supplied (HR-03).

**Verify Phase 6:** ✅ runtime driver confirms `reloadByRealPath` re-runs the loader for a matching path (and ignores non-matches), and the `FileWatcher` detects a real on-disk modification under `EMBER_HOT_RELOAD`; Release compiles with the watcher inert. Live `.png`-edit-in-sandbox is the user's build.

---

## Phase 7 — Epic 04 Migration

Implements `MIG-01 … MIG-05`, `SBX-01 … SBX-04`.

- [x] Changed `SpriteRenderer.texture` from `std::shared_ptr<ITexture2D>` to `AssetHandle<Texture2D>`; `texturePath` stays the serialized source (MIG-01). `Components2D.h` includes `ember/assets/AssetHandle.h`/`Texture2D.h`; `ember_renderer_2d` links `ember_assets`.
- [x] `SpriteRenderSystem` now takes an `AssetManager&`, resolves `get<Texture2D>(sprite.texture)`, falls back to the missing-texture placeholder when set-but-not-resident, and passes the bindable `ITexture2D` to the new `Renderer2D::drawSprite(..., texture, ...)` overload (MIG-03).
- [x] Register `SpriteRenderer` + `Camera2D` with the serializer (MIG-04). Extracted the YAML field visitors + `makeMeta` into a public `ember/serialization/ComponentSerialization.h` exposing `registerComponentType<T>(name)` (the private `SerializationDetail.h` now includes it — single source of truth, no cycle). `renderer_2d` gained one TU `RenderComponentSerialization.cpp` (links `ember_serialization`/yaml-cpp **PRIVATE**) exposing `void registerRenderComponents()`. **Call it once at startup** (e.g. beside `Renderer2D::init()`) so sprites/cameras round-trip in `.escene`. yaml stays out of all renderer public headers.
- [~] Sandbox demo (asset-loaded sprite, input-driven camera, `"ui"` context, hot-reload) — **deferred**: GL-runtime only and would rewrite the working Epic-04 demo. Engine-side pieces it needs (Input facade, `AssetManager`, `processPendingUploads`, `pollAssetHotReload`) are all in place. (SBX-01..04, MIG-05)

**Verify Phase 7:** ✅ `Components2D.h`, `Renderer2D` (`drawSprite` overload), and `SpriteRenderSystem` compile clean `-Werror`; sandbox uses `drawQuad` so the signature change is safe. Visual sandbox + scene round-trip are the user's build.

---

## Phase 8 — Tests wiring, verification

- [x] Added `tests/input/TestInputState.cpp` and `tests/assets/TestAssetManager.cpp` to `ember_tests` (the `ember` aggregate links `ember_input`/`ember_assets`). Asset suite covers handle/cache/refcount/pin/reload/missing-loader, `.meta` round-trip, async-via-`processPendingUploads`, and `reloadByRealPath`.
- [~] `ctest --preset debug`/`release` green — **user's build** (sandbox prebuilt Catch2/spdlog/yaml are MinGW objects, unlinkable with Linux GCC; logic verified via standalone Linux drivers instead).
- [x] Header hygiene verified: no GLFW in `engine/input/include` and no miniaudio/stb/yaml in `engine/assets/include` (only comments + the intentional opaque `struct GLFWwindow;` forward decl).
- [~] `sanitize` preset clean (NFR-03) — user's build; eviction logic is leak-safe by construction (shared_ptr data + unload thunk).
- [~] Push; CI green on all three platforms, zero warnings — user's build.

### Acceptance checklist (from `02-requirements.md` §12)

- [ ] Key edge states correct across 3 frames.
- [ ] Action map (keyboard + gamepad) loads from YAML and responds.
- [ ] `load<Texture2D>` caches by path (single loader call).
- [ ] `loadAsync` doesn't block the main thread; texture appears on completion.
- [ ] Hot reload updates a texture within ~1 frame (Debug).
- [ ] Releasing all handles evicts the asset (sanitize-clean).
- [ ] `SpriteRenderer` uses `AssetHandle<Texture2D>`; sprites render + round-trip.
- [ ] All tests pass; CI green, zero warnings.

---

## Suggested Commit Sequence

1. `Epic 05: scaffold input/assets modules + miniaudio + CMake` (Phase 0)
2. `Epic 05: input enums, polling, manager` (Phase 1)
3. `Epic 05: action map, context stack, gamepad` (Phase 2)
4. `Epic 05: asset handles, manager, database` (Phase 3)
5. `Epic 05: asset loaders + relocate texture/font from renderer` (Phase 4)
6. `Epic 05: async loading + pending GL uploads` (Phase 5)
7. `Epic 05: hot reload (FileWatcher)` (Phase 6)
8. `Epic 05: migrate SpriteRenderer to AssetHandle; input-driven sandbox` (Phase 7)
9. `Epic 05: tests, sandbox, verification` (Phase 8)

Each commit should leave the build green and CI passing.
