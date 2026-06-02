# Epic 05 — Input & Asset Management

**Goal:** Replace the stub input and asset handling from earlier epics with production-quality systems. The input system is polling-friendly and action-mapped. The asset manager provides handle-based ownership, reference counting, hot-reload, and async loading.

**Depends on:** 02-foundation, 03-ecs-and-scene  
**Blocks:** 06-scripting, 07-editor-v1

---

## Scope

### Input System (`engine/input/`)

#### State Polling API
- `Input::isKeyDown(Key)` — held this frame
- `Input::isKeyPressed(Key)` — went down this frame
- `Input::isKeyReleased(Key)` — went up this frame
- `Input::isMouseButtonDown/Pressed/Released(Mouse)`
- `Input::getMousePosition()` → vec2 (screen space)
- `Input::getMouseDelta()` → vec2
- `Input::getMouseScroll()` → f32

#### Key / Mouse Enums
- `Key` enum covering full GLFW keycode set (mapped from GLFW constants at the platform boundary — upper layers never see GLFW types)
- `Mouse` enum: Left, Right, Middle, Button4, Button5

#### Action Map
- `InputAction` — named action bound to one or more physical inputs
  - `bindKey(Key)`
  - `bindMouseButton(Mouse)`
  - `bindAxis(GamepadAxis, scale)`
- `InputMap` — collection of actions; can be loaded from YAML
- `Input::getAction(name)` → `ActionState { pressed, held, released, axisValue }`
- Persistence: action maps saved to `settings/input.yaml`

#### Input Context Stack
- `InputManager::pushContext(name)` / `popContext()` — only the top context's bindings are active
- Built-in contexts: `"game"`, `"editor"`, `"ui"`
- Prevents game input firing while editor panels capture keyboard

#### Gamepad
- Single gamepad supported (index 0); enumeration deferred
- `Input::getGamepadAxis(GamepadAxis)` → f32 (-1..1)
- `Input::isGamepadButtonDown(GamepadButton)`

### Asset Manager (`engine/assets/`)

#### Handle System
- `AssetHandle<T>` — 64-bit asset ID; copyable, cheap; null handle is `AssetHandle<T>::null()`
- `AssetManager` (singleton-style, injected via `Application`) owns all live asset data
  - `load<T>(virtualPath)` → `AssetHandle<T>` (synchronous; blocks if not cached)
  - `loadAsync<T>(virtualPath)` → `std::future<AssetHandle<T>>`
  - `get<T>(handle)` → `T*` (returns nullptr if not loaded)
  - `release(handle)` — decrements refcount; asset evicted when count reaches 0
  - `pin(handle)` — prevents eviction regardless of refcount
  - `reload(handle)` — force re-execute loader (hot reload trigger)

#### Asset Database
- `AssetDatabase` — maps virtual paths (e.g., `"textures/player.png"`) to real filesystem paths and `.meta` sidecar data
- `.meta` file format (YAML):
  ```yaml
  uuid: 8f3a2b1c...
  type: Texture2D
  importer: Texture2DImporter
  settings:
    filter: Linear
    wrap: Repeat
    generateMips: true
  ```
- `AssetDatabase::scan(rootPath)` — walks the assets directory, creates `.meta` for new files
- `AssetDatabase::getPath(UUID)` → filesystem path (for serialization by UUID rather than path)

#### Asset Loaders
Loaders registered at startup:

| Type         | Loader               |
|--------------|----------------------|
| Texture2D    | Texture2DLoader (stb) |
| AudioClip    | AudioClipLoader (miniaudio) |
| Font         | FontLoader (stb_truetype) |
| Scene        | YAMLSceneLoader       |
| Shader       | GLSLShaderLoader      |

- `IAssetLoader<T>` interface: `load(path, settings)` → T, `unload(T&)`
- Loaders run on a background thread pool for async loads; GL uploads happen on main thread via a `PendingUpload` queue drained each frame

#### Hot Reload
- `FileWatcher` monitors `assets/` directory (platform: `inotify` on Linux, `ReadDirectoryChangesW` on Windows, `kqueue` on macOS)
- On file change: `AssetManager::reload(handle)` queued for next-frame execution
- Subscribers of `AssetReloadedEvent<T>` are notified after reload completes

---

## Exit Criteria

- [ ] `Input::isKeyPressed`, `isKeyDown`, `isKeyReleased` return correct values across 3 consecutive frames for the same key
- [ ] Action map with keyboard + gamepad bindings loads from YAML and responds correctly
- [ ] `AssetManager::load<Texture2D>` returns valid handle; same path returns cached handle (single load confirmed via loader call count)
- [ ] `loadAsync` completes without blocking the main thread; texture renders correctly on completion
- [ ] Hot reload: modify a `.png` file on disk while sandbox is running; texture updates within 1 frame of file write
- [ ] Releasing all handles to an asset evicts it from memory (verified via allocator stats)

---

## Key Files Created

```
engine/input/include/ember/input/Input.h
engine/input/include/ember/input/InputManager.h
engine/input/include/ember/input/InputAction.h
engine/input/include/ember/input/Keys.h
engine/input/InputManager.cpp
engine/assets/include/ember/assets/AssetManager.h
engine/assets/include/ember/assets/AssetHandle.h
engine/assets/include/ember/assets/AssetDatabase.h
engine/assets/include/ember/assets/IAssetLoader.h
engine/assets/loaders/Texture2DLoader.h/.cpp
engine/assets/loaders/FontLoader.h/.cpp
engine/assets/loaders/AudioClipLoader.h/.cpp
engine/assets/FileWatcher.h/.cpp
tests/input/TestInputState.cpp
tests/assets/TestAssetManager.cpp
settings/input.yaml
```

---

## Notes & Decisions

- GLFW key/mouse codes are translated to Ember's `Key`/`Mouse` enums at the window event callback boundary — no GLFW types leak past `engine/platform/`
- The `InputContext` stack intentionally does not propagate input downward — only the top context fires; this avoids "double-firing" bugs when UI overlaps the game viewport
- Audio playback (using miniaudio) is deferred to a stretch goal in this epic; the `AudioClip` asset type and loader are implemented but the `AudioSystem` is a stub
- `FileWatcher` is not implemented for Web/Emscripten — hot reload is a dev-only feature guarded by `EMBER_HOT_RELOAD` compile flag
