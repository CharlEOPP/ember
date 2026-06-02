# Epic 02 — Foundation

**Goal:** Establish the build system, core utilities, platform window, and a verified rendering context. Every subsequent epic builds on this base. Nothing is shipped here — it is the skeleton everything else hangs on.

**Depends on:** 01-vision.md  
**Blocks:** All other epics

---

## Scope

### CMake Project Skeleton
- Root `CMakeLists.txt` with compiler options (C++20, warnings-as-errors, sanitizer configs)
- `cmake/` modules: `CompilerOptions.cmake`, `Dependencies.cmake`, `Platform.cmake`
- FetchContent declarations for: GLFW, glad, glm, EnTT, spdlog, yaml-cpp, stb, Catch2, ImGui
- All subsystem targets defined as proper CMake static libraries with explicit include dirs
- Install targets and `libember` INTERFACE aggregate target
- CI pipeline: GitHub Actions matrix (Windows MSVC, Linux GCC, macOS Clang) — all three must build clean

### Core Library (`engine/core/`)
- Primitive type aliases: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64`
- `UUID` generation (xoshiro256++ based, no stdlib random)
- `StringHash` — compile-time FNV-1a hash of string literals
- `Result<T, E>` wrapping `std::expected<T, E>`
- `EMBER_ASSERT(cond, msg, ...)` — breaks in debug, logs + terminates in release
- `EMBER_VERIFY(cond, msg, ...)` — assert that survives release builds
- Logging: spdlog wrapper exposing `ember` and `game` channels
  - `EMBER_LOG_TRACE/DEBUG/INFO/WARN/ERROR/FATAL`
  - `GAME_LOG_INFO(...)` etc.
  - Console sink + rotating file sink
- `ArenaAllocator` — bump allocator with reset; used for frame-lifetime allocations
- `PoolAllocator<T>` — fixed-size object pool

### Platform Layer (`engine/platform/`)
- `Window` class wrapping GLFW
  - `WindowSpec` (title, width, height, vsync, fullscreen)
  - `pollEvents()`, `swapBuffers()`, `shouldClose()`
  - Event callback slot (`std::function<void(Event&)>`)
- High-resolution `Timer` class (nanosecond precision via `std::chrono`)
- `FileSystem` utility (thin `std::filesystem` wrapper with virtual path helpers)
- Platform detection macros: `EMBER_PLATFORM_WINDOWS`, `EMBER_PLATFORM_LINUX`, `EMBER_PLATFORM_MACOS`

### RHI — Minimal Stub (`engine/renderer/api/`)
- Abstract interfaces: `IVertexBuffer`, `IIndexBuffer`, `IShader`, `ITexture2D`, `IFramebuffer`
- `RHI::init(backend)` factory; OpenGL 4.6/4.1 backend only
- Enough to clear the screen and draw a hard-coded triangle with a flat-color shader
- `RHI::setViewport`, `RHI::clear`, `RHI::drawIndexed`

### Test Harness
- Catch2 integrated, at least one passing test per core utility (UUID, StringHash, ArenaAllocator, Logger)
- Tests run as part of CI

---

## Exit Criteria

- [ ] `cmake --preset release` builds successfully on Windows, Linux, macOS
- [ ] A white triangle renders in the sandbox window at 60fps
- [ ] All Catch2 tests pass
- [ ] No warnings at `-Wall -Wextra` on any platform
- [ ] CI is green on first commit

---

## Key Files Created

```
CMakeLists.txt
cmake/CompilerOptions.cmake
cmake/Dependencies.cmake
engine/core/include/ember/core/Types.h
engine/core/include/ember/core/Assert.h
engine/core/include/ember/core/Log.h
engine/core/include/ember/core/UUID.h
engine/core/include/ember/core/StringHash.h
engine/core/include/ember/core/Arena.h
engine/platform/include/ember/platform/Window.h
engine/platform/include/ember/platform/Timer.h
engine/renderer/api/include/ember/renderer/RHI.h
engine/renderer/api/opengl/OpenGLBuffer.h
engine/renderer/api/opengl/OpenGLShader.h
sandbox/main.cpp
tests/core/TestUUID.cpp
tests/core/TestArena.cpp
.github/workflows/ci.yml
```

---

## Notes & Decisions

- Use `glad2` (not glad legacy) for OpenGL loading — supports OpenGL 4.6 core profile
- GLM is header-only, included directly; no wrapper needed
- spdlog is used as a compiled library (not header-only) to keep compile times down
- Do **not** add any game logic, components, or systems here — this epic ends at "triangle on screen"
