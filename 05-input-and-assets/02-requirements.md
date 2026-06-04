# Epic 05 — Input & Asset Management: Requirements

**Status:** Draft  
**Epic:** 05-input-and-assets  
**Depends on:** 02-foundation, 03-ecs-and-scene, 04-renderer-2d, 01-vision.md §8/§9  
**Blocks:** 06-scripting, 07-editor-v1

---

## 1. Overview

This epic replaces the stub input and ad-hoc texture loading from earlier epics with two production systems:

1. An **Input system**: a polling-friendly API over keyboard/mouse/gamepad, an action-mapping layer loadable from YAML, and an input-context stack (game vs. editor vs. UI).
2. An **Asset manager**: handle-based, reference-counted ownership with synchronous and async loading, a virtual-path → file database with `.meta` sidecars, type-specific loaders, and dev-time hot reload.

It also performs the **Epic 04 migration**: the texture/font loaders move from `engine/renderer/2d/loaders/` into `engine/assets/`, and `SpriteRenderer` switches from a raw `std::shared_ptr<ITexture2D>` to an `AssetHandle<Texture2D>`.

The measure of success: keyboard/gamepad input drives the sandbox (camera pan/zoom), and a texture loaded by virtual path renders, is cached on second load, loads asynchronously without a frame hitch, and hot-reloads when the file changes on disk.

### 1.1 New dependency

`miniaudio` (single-header) is added via `FetchContent` for the `AudioClip` loader. Audio *playback* (`AudioSystem`) is a **stub** this epic; only the asset type + loader are real.

---

## 2. Definitions

| Term | Meaning |
|------|---------|
| **Pressed / Held / Released** | edge-down this frame / down this frame / edge-up this frame |
| **Action** | a named, rebindable input (`"jump"`) bound to one or more physical inputs |
| **Input context** | a named layer on a stack; only the top context's bindings fire |
| **AssetHandle\<T\>** | a cheap, copyable 64-bit id referring to a managed asset of type `T` |
| **Virtual path** | an asset path relative to the assets root, e.g. `"textures/player.png"` |
| **`.meta`** | a YAML sidecar describing an asset's UUID, type, importer, and import settings |
| **Refcount / pin** | handles are reference-counted; pinning prevents eviction regardless of count |
| **Hot reload** | re-running a loader when its source file changes on disk, swapping data behind live handles |
| **PendingUpload** | a GPU upload deferred from a worker thread to the main thread (GL is single-threaded) |

---

## 3. Input — State Polling (`engine/input/`)

### 3.1 Keys & Mouse enums (`Keys.h`)

| ID | Requirement |
|----|-------------|
| INP-01 | A `Key` enum SHALL cover the standard keyboard set (letters, digits, function keys, modifiers, arrows, space, escape, enter, etc.), defined independently of GLFW. |
| INP-02 | A `Mouse` enum SHALL define `Left`, `Right`, `Middle`, `Button4`, `Button5`. |
| INP-03 | GLFW key/mouse codes SHALL be translated to `Key`/`Mouse` at the platform window-event boundary; no GLFW type or constant SHALL appear in any `engine/input` public header (consistent with Epic 02). |

### 3.2 Polling API (`Input.h`)

| ID | Requirement |
|----|-------------|
| INP-04 | `Input::isKeyDown(Key)`, `isKeyPressed(Key)`, `isKeyReleased(Key)` SHALL return held / edge-down / edge-up state for the current frame. |
| INP-05 | `Input::isMouseButtonDown/Pressed/Released(Mouse)` SHALL behave analogously for mouse buttons. |
| INP-06 | `Input::getMousePosition()` SHALL return the cursor position in screen pixels (`glm::vec2`); `getMouseDelta()` the movement since last frame; `getMouseScroll()` the scroll delta this frame. |
| INP-07 | Edge states (`pressed`/`released`) SHALL be computed from current vs. previous-frame state, updated exactly once per frame by `InputManager::update()`. |
| INP-08 | `Input` SHALL be a thin static facade over the active `InputManager`; querying input before the manager exists SHALL be safe (returns "not pressed"). |

### 3.3 Input update flow

| ID | Requirement |
|----|-------------|
| INP-09 | The platform `Window` event callback SHALL feed key/mouse events into the `InputManager` (which also re-broadcasts them as the Epic 03 `KeyPressedEvent`/`MouseMovedEvent`/etc. on the `EventBus`). |
| INP-10 | `InputManager::update()` SHALL be called once per frame (after `pollEvents`) to roll current state into previous state and recompute edges. |

---

## 4. Input — Action Map (`InputAction.h`)

| ID | Requirement |
|----|-------------|
| ACT-01 | `InputAction` SHALL be a named action with `bindKey(Key)`, `bindMouseButton(Mouse)`, and `bindAxis(GamepadAxis, scale)`; an action MAY have multiple bindings. |
| ACT-02 | An `InputMap` SHALL hold a collection of named actions, keyed by `StringHash` (Epic 02). |
| ACT-03 | `Input::getAction(name)` SHALL return `ActionState { bool pressed, held, released; f32 axisValue }`, aggregating all of the action's bindings (any binding down ⇒ held; max-magnitude axis wins). |
| ACT-04 | An `InputMap` SHALL load from and save to YAML at `settings/input.yaml`. |
| ACT-05 | Loading a malformed or missing input map SHALL log a warning and fall back to built-in defaults rather than failing. |

---

## 5. Input — Context Stack & Gamepad

### 5.1 Context stack

| ID | Requirement |
|----|-------------|
| CTX-01 | `InputManager::pushContext(name)` / `popContext()` SHALL manage a stack of named contexts; built-ins SHALL include `"game"`, `"editor"`, `"ui"`. |
| CTX-02 | Only the **top** context's action bindings SHALL resolve; lower contexts SHALL NOT fire (no downward propagation), preventing game input from firing while the editor/UI captures input. |
| CTX-03 | Raw polling (`Input::isKeyDown`) SHALL remain available regardless of context (contexts gate *actions*, not raw queries). |

### 5.2 Gamepad

| ID | Requirement |
|----|-------------|
| GP-01 | A single gamepad (index 0) SHALL be supported; multi-gamepad enumeration is deferred. |
| GP-02 | `Input::getGamepadAxis(GamepadAxis)` SHALL return `-1..1`; a configurable dead-zone SHALL be applied. |
| GP-03 | `Input::isGamepadButtonDown(GamepadButton)` SHALL report button state; `GamepadAxis`/`GamepadButton` SHALL be GLFW-free enums. |
| GP-04 | Gamepad absence SHALL be handled gracefully (axes read 0, buttons read not-down). |

---

## 6. Asset Manager (`engine/assets/`)

### 6.1 Handles (`AssetHandle.h`)

| ID | Requirement |
|----|-------------|
| AM-01 | `AssetHandle<T>` SHALL be a 64-bit id, trivially copyable and cheap to pass by value. |
| AM-02 | `AssetHandle<T>::null()` SHALL return a sentinel invalid handle; `valid()` SHALL report whether a handle is non-null. |
| AM-03 | `AssetHandle<T>` SHALL be equality-comparable and usable as a map key. |

### 6.2 Manager API (`AssetManager.h`)

| ID | Requirement |
|----|-------------|
| AM-04 | `load<T>(virtualPath)` SHALL return an `AssetHandle<T>`, loading synchronously (blocking) if not already cached. |
| AM-05 | Loading the **same** virtual path again SHALL return the cached handle without re-running the loader (verified by loader call count). |
| AM-06 | `loadAsync<T>(virtualPath)` SHALL return `std::future<AssetHandle<T>>` and SHALL NOT block the calling (main) thread; the data is loaded on a worker thread. |
| AM-07 | `get<T>(handle)` SHALL return `T*`, or `nullptr` if the handle is invalid or the asset is not yet resident. |
| AM-08 | `release(handle)` SHALL decrement the refcount; when it reaches 0 the asset SHALL be evicted and its loader's `unload` invoked (unless pinned). |
| AM-09 | `pin(handle)` SHALL prevent eviction regardless of refcount; `unpin(handle)` SHALL restore normal behaviour. |
| AM-10 | `reload(handle)` SHALL re-run the loader and swap the data behind the handle transparently (the handle value SHALL NOT change). |
| AM-11 | GPU-backed assets (textures) loaded on a worker thread SHALL defer the GL upload to the main thread via a `PendingUpload` queue drained once per frame; `get<T>` SHALL return `nullptr` until the upload completes. |

### 6.3 Asset Database (`AssetDatabase.h`)

| ID | Requirement |
|----|-------------|
| ADB-01 | `AssetDatabase` SHALL map virtual paths to real filesystem paths and to `.meta` sidecar data. |
| ADB-02 | The `.meta` format SHALL be YAML containing at least: `uuid`, `type`, `importer`, and a `settings` map (e.g. texture filter/wrap/mips). |
| ADB-03 | `AssetDatabase::scan(rootPath)` SHALL walk the assets directory and create a `.meta` (with a fresh UUID) for any asset lacking one. |
| ADB-04 | `AssetDatabase::getPath(UUID)` SHALL resolve a UUID to a filesystem path, enabling scene serialization to reference assets by stable UUID rather than path. |

### 6.4 Loaders (`IAssetLoader.h` + `loaders/`)

| ID | Requirement |
|----|-------------|
| LOAD-01 | `IAssetLoader<T>` SHALL declare `load(path, settings) → T` and `unload(T&)`. |
| LOAD-02 | Loaders SHALL be registered with the manager at startup; required loaders this epic: `Texture2D`, `Font`, `Scene` (YAML), `Shader` (GLSL), and `AudioClip` (miniaudio). |
| LOAD-03 | The **Texture2D and Font loaders SHALL be relocated** from `engine/renderer/2d/loaders/` to `engine/assets/loaders/` (carry-over from Epic 04), reusing their existing stb-based implementations. |
| LOAD-04 | Loaders used by `loadAsync` SHALL run on a background thread pool; non-thread-safe steps (GL uploads) SHALL be marshalled to the main thread per AM-11. |

### 6.5 Hot Reload (`FileWatcher.h`)

| ID | Requirement |
|----|-------------|
| HR-01 | A `FileWatcher` SHALL monitor the `assets/` directory using the platform API (`ReadDirectoryChangesW` / `inotify` / `kqueue`). |
| HR-02 | On a watched file change, `AssetManager::reload` SHALL be queued and executed on the next frame. |
| HR-03 | After a reload completes, subscribers of `AssetReloadedEvent` SHALL be notified (via the Epic 03 `EventBus`). |
| HR-04 | Hot reload SHALL be a dev-only feature behind an `EMBER_HOT_RELOAD` compile flag (default ON in Debug, OFF in Release); with it off, `FileWatcher` SHALL compile to a no-op. |

---

## 7. Epic 04 Migration (renderer integration)

| ID | Requirement |
|----|-------------|
| MIG-01 | `SpriteRenderer.texture` SHALL change from `std::shared_ptr<ITexture2D>` to `AssetHandle<Texture2D>`; the serializable `texturePath` field SHALL drive loading on scene load. |
| MIG-02 | `Texture2D` SHALL be the asset type wrapping an `ITexture2D` (the RHI texture); `AssetManager::get<Texture2D>(handle)` SHALL yield the bindable texture for the renderer. |
| MIG-03 | `SpriteRenderSystem` SHALL resolve each sprite's `AssetHandle<Texture2D>` via the `AssetManager` and pass the bindable texture to `Renderer2D` (treating a not-yet-resident handle as the missing-texture placeholder, not a crash). |
| MIG-04 | `SpriteRenderer` and `Camera2D` SHALL be registered with the serializer (Epic 03 `ComponentRegistry`) so sprites/cameras round-trip in `.escene` files, with textures referenced by virtual path/UUID. |
| MIG-05 | The migration SHALL not regress the Epic 04 sandbox: the sprite batch still renders (now via handle-resolved textures). |

---

## 8. Sandbox Requirements

| ID | Requirement |
|----|-------------|
| SBX-01 | The sandbox SHALL drive the `Camera2D`/`EditorCamera` pan and zoom from real input (keyboard and/or mouse) via the new `Input` API — completing the interactive part of Epic 04's camera exit criterion. |
| SBX-02 | The sandbox SHALL load at least one texture through `AssetManager::load<Texture2D>("…")` (virtual path) and render sprites using it. |
| SBX-03 | The sandbox SHALL demonstrate hot reload: editing the texture file on disk updates the on-screen sprites within ~1 frame (Debug builds). |
| SBX-04 | Pushing an `"editor"`/`"ui"` context SHALL visibly suppress the game camera controls while active. |

---

## 9. Test Requirements (`tests/`)

| ID | Requirement |
|----|-------------|
| TST-01 | New tests SHALL join the existing `ember_tests` target; pure-CPU tests SHALL run headless on CI (no window/GL/gamepad). |

### 9.1 Required test cases

| Test | Verifies | Req |
|------|----------|-----|
| `Input_EdgeStatesAcrossFrames` | Feeding a synthetic key down→hold→up across 3 frames yields correct `pressed`/`held`/`released` each frame | INP-04/07 |
| `Input_MouseDeltaAndScroll` | Delta and scroll reset correctly each frame | INP-06/07 |
| `ActionMap_AggregatesBindings` | An action bound to two keys reports held when either is down; axis aggregation picks max magnitude | ACT-01/03 |
| `ActionMap_YamlRoundtrip` | An `InputMap` saved to YAML and reloaded produces identical bindings | ACT-04 |
| `Context_TopOnlyFires` | With `"ui"` pushed over `"game"`, a game-context action does not fire; popping restores it | CTX-01/02 |
| `AssetHandle_NullAndEquality` | `null()` is invalid; equal handles compare equal; usable in a map | AM-01/02/03 |
| `AssetManager_CachesByPath` | Loading the same path twice runs the loader once and returns the same handle | AM-05 |
| `AssetManager_RefcountEvicts` | Releasing the last handle evicts the asset and calls the loader's `unload` | AM-08 |
| `AssetDatabase_MetaRoundtrip` | `.meta` write/read preserves uuid/type/settings; `scan` creates `.meta` for a new file | ADB-02/03 |

> Tests use **mock loaders** (CPU-only, counting calls) so they run without GL/disk-watching. Async/hot-reload/GL-upload behaviour is verified in the sandbox.

---

## 10. Non-Functional Requirements

| ID | Requirement |
|----|-------------|
| NFR-01 | **No main-thread stall.** `loadAsync` of a large texture SHALL not drop a frame on the main thread (work happens on the pool; only the bounded `PendingUpload` drain runs on main). |
| NFR-02 | **Thread safety.** `AssetManager` internal maps SHALL be guarded; concurrent `loadAsync` calls SHALL be safe. |
| NFR-03 | **Deterministic eviction.** An asset SHALL be freed exactly once when its refcount hits 0 and it is unpinned (no leaks, no double-free) — verified under the `sanitize` preset. |
| NFR-04 | **Header hygiene.** No GLFW in `engine/input` public headers; no miniaudio/stb/yaml in `engine/assets` *public* headers (implementation detail in `.cpp`). |
| NFR-05 | **Warning-free.** All new code compiles clean under `-Wall -Wextra -Wpedantic -Werror` (GCC/Clang) and `/W4 /WX` (MSVC) on all three CI platforms; third-party single-headers (miniaudio) compiled with warnings disabled like stb. |
| NFR-06 | **Input latency.** Polled input SHALL reflect events from the same frame's `pollEvents` (no one-frame lag for `isKeyDown`). |

---

## 11. Out of Scope

- **Audio playback / `AudioSystem`** — only the `AudioClip` asset type + loader are built; sound output is deferred.
- **Multi-gamepad enumeration and rebinding UI** — single gamepad only.
- **Asset hot-reload on Web/Emscripten** — `FileWatcher` is desktop-only.
- **Binary/compiled asset packs** — virtual-path + loose files only this epic.
- **Touch / pen input** — keyboard, mouse, single gamepad only.
- **Networked or replayed input** — local devices only.

---

## 12. Acceptance Criteria Summary

The epic is complete when ALL of the following are true:

1. `isKeyPressed`/`isKeyDown`/`isKeyReleased` return correct values across 3 consecutive frames for the same key.
2. An action map with keyboard + gamepad bindings loads from YAML and responds correctly.
3. `AssetManager::load<Texture2D>` returns a valid handle; the same path returns the cached handle (single loader call confirmed).
4. `loadAsync` completes without blocking the main thread; the texture renders correctly on completion.
5. Hot reload: editing a `.png` on disk while the sandbox runs updates the texture within ~1 frame (Debug).
6. Releasing all handles to an asset evicts it (verified via allocator/refcount stats; clean under `sanitize`).
7. `SpriteRenderer` now uses `AssetHandle<Texture2D>`; sprites still render and round-trip through `.escene` serialization.
8. All Catch2 tests pass (debug + release); CI green on all three platforms with zero warnings.
