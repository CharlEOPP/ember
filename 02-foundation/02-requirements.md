# Epic 02 — Foundation: Requirements

**Status:** Draft  
**Epic:** 02-foundation  
**Depends on:** 01-vision.md  
**Blocks:** All subsequent epics

---

## 1. Overview

This document specifies the complete requirements for the Foundation epic. The foundation provides:

1. A CMake build system that compiles cleanly across Windows, Linux, and macOS
2. A `core` library of primitive types, utilities, logging, and allocators
3. A `platform` library abstracting the OS window, timer, and filesystem
4. A minimal Render Hardware Interface (RHI) stub with an OpenGL backend
5. A sandbox application that proves the above by rendering a triangle
6. A CI pipeline that keeps the above honest

Nothing game-specific is built here. The measure of success is a clean, warning-free multi-platform build and a triangle on screen.

---

## 2. Definitions

| Term | Meaning |
|------|---------|
| **RHI** | Render Hardware Interface — the backend-agnostic GPU abstraction layer |
| **Sandbox** | The developer test application that lives in `sandbox/`; not a shipped product |
| **Target** | A CMake build target (library, executable, or interface) |
| **Preset** | A CMake configure preset defined in `CMakePresets.json` |
| **UB** | Undefined behaviour |
| **CI** | Continuous Integration (GitHub Actions) |

---

## 3. Build System Requirements

### 3.1 CMake Version & Language Standard

| ID | Requirement |
|----|-------------|
| BS-01 | The minimum required CMake version SHALL be 3.25. |
| BS-02 | All targets SHALL compile as C++20 (`cxx_std_20` via `target_compile_features`). |
| BS-03 | The C++ standard SHALL be set to `CXX_STANDARD_REQUIRED ON` and `CXX_EXTENSIONS OFF` on every target. |

### 3.2 CMake Presets

| ID | Requirement |
|----|-------------|
| BS-04 | A `CMakePresets.json` file SHALL define at minimum the following configure presets: `debug`, `release`, `relwithdebinfo`, `sanitize`. |
| BS-05 | `cmake --preset debug` SHALL produce a build directory at `build/debug/`. |
| BS-06 | `cmake --preset release` SHALL produce a build directory at `build/release/`. |
| BS-07 | Each configure preset SHALL have a corresponding build preset so that `cmake --build --preset <name>` works. |

### 3.3 Compiler Warning Policy

| ID | Requirement |
|----|-------------|
| BS-08 | All engine and sandbox targets SHALL compile with zero warnings on MSVC (`/W4`), GCC (`-Wall -Wextra -Wpedantic`), and Clang (`-Wall -Wextra -Wpedantic`). |
| BS-09 | Warnings SHALL be treated as errors (`/WX` on MSVC, `-Werror` on GCC/Clang) for all engine targets. |
| BS-10 | Third-party targets fetched via FetchContent SHALL be excluded from the warning-as-error policy using `SYSTEM` include directories. |

### 3.4 Compiler Options Module (`cmake/CompilerOptions.cmake`)

| ID | Requirement |
|----|-------------|
| BS-11 | `CompilerOptions.cmake` SHALL define a CMake function `ember_set_compiler_options(target)` that applies the standard warning flags for the detected compiler. |
| BS-12 | The function SHALL detect the compiler via `CMAKE_CXX_COMPILER_ID` and apply the correct flag set (`MSVC`, `GNU`, `Clang`, `AppleClang`). |
| BS-13 | The `sanitize` preset SHALL add `-fsanitize=address,undefined` (GCC/Clang) or `/fsanitize=address` (MSVC) to compile and link flags. |

### 3.5 Dependency Management (`cmake/Dependencies.cmake`)

| ID | Requirement |
|----|-------------|
| BS-14 | All third-party dependencies SHALL be declared via CMake `FetchContent` in `cmake/Dependencies.cmake`. No Git submodules. |
| BS-15 | The following dependencies SHALL be declared with pinned version tags or commit hashes (not `HEAD` or `main`): |
|       | — GLFW 3.4 |
|       | — glad2 (generated for OpenGL 4.6 core + OpenGL 4.1 core; both profiles) |
|       | — GLM 1.0.1 |
|       | — EnTT 3.13 |
|       | — spdlog 1.14 |
|       | — yaml-cpp 0.8 |
|       | — stb (pinned commit) |
|       | — Catch2 3.6 |
|       | — Dear ImGui 1.90 (docking branch) |
| BS-16 | `FetchContent_MakeAvailable` SHALL be called for all dependencies in a single block to allow CMake to parallelise downloads. |
| BS-17 | A CMake cache variable `EMBER_FETCH_DEPS` (default `ON`) SHALL gate the `FetchContent` block so it can be disabled when dependencies are pre-installed. |

### 3.6 Target Structure

| ID | Requirement |
|----|-------------|
| BS-18 | The following CMake targets SHALL exist after configuration: `ember_core`, `ember_platform`, `ember_renderer_api`, `ember_sandbox`, `ember_tests`. |
| BS-19 | A CMake INTERFACE library target `ember` SHALL be defined that links all engine subsystem targets, so that external consumers can link against a single target. |
| BS-20 | Each subsystem target SHALL declare its own include directories with `PUBLIC` scope for headers that are part of its public API and `PRIVATE` scope for internal implementation headers. |
| BS-21 | No target SHALL use `include_directories()` (directory-scoped); all includes SHALL be target-scoped via `target_include_directories()`. |

### 3.7 Continuous Integration

| ID | Requirement |
|----|-------------|
| BS-22 | A GitHub Actions workflow file SHALL exist at `.github/workflows/ci.yml`. |
| BS-23 | The CI matrix SHALL include all three of: `windows-latest` (MSVC), `ubuntu-latest` (GCC), `macos-latest` (AppleClang). |
| BS-24 | Each CI job SHALL: configure, build (Release preset), and run `ctest`. |
| BS-25 | The CI workflow SHALL cache the FetchContent download directory to avoid re-downloading on every run. |
| BS-26 | A failing CI run SHALL block merging to `main` (branch protection rule — documented, not enforced by this codebase). |

---

## 4. Core Library Requirements (`engine/core/`)

### 4.1 Primitive Types (`Types.h`)

| ID | Requirement |
|----|-------------|
| CORE-01 | The following type aliases SHALL be defined in `ember/core/Types.h`: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64`. |
| CORE-02 | Each alias SHALL correspond to the exact-width integer type from `<cstdint>` or a floating-point type: `u8` = `uint8_t`, `i32` = `int32_t`, `f32` = `float`, `f64` = `double`. |
| CORE-03 | A static assert SHALL verify the size of each alias at compile time (e.g., `static_assert(sizeof(f32) == 4)`). |
| CORE-04 | All type aliases SHALL be defined inside the `ember` namespace. |
| CORE-05 | The header SHALL be self-contained (compilable with no other Ember header included). |

### 4.2 UUID (`UUID.h`)

| ID | Requirement |
|----|-------------|
| CORE-06 | `UUID` SHALL be a type alias for `u64` (`uint64_t`). |
| CORE-07 | A function `UUID generateUUID()` SHALL return a new unique identifier on each call. |
| CORE-08 | The generator SHALL use the **xoshiro256++** algorithm seeded from a hardware entropy source (`std::random_device`) at program startup. |
| CORE-09 | The null UUID value SHALL be `0`. A constant `UUID UUID::null = 0` (or equivalent) SHALL be provided. |
| CORE-10 | `generateUUID()` SHALL never return `0`. |
| CORE-11 | Two calls to `generateUUID()` in the same process SHALL NOT return the same value (tested over 1 000 000 iterations in the test suite). |
| CORE-12 | `generateUUID()` SHALL be thread-safe (internal state protected by `std::atomic` or equivalent). |

### 4.3 StringHash (`StringHash.h`)

| ID | Requirement |
|----|-------------|
| CORE-13 | `StringHash` SHALL be a type wrapping a `u64` hash value. |
| CORE-14 | The hash function SHALL be **FNV-1a 64-bit**. |
| CORE-15 | The `""_sh` user-defined literal operator SHALL be provided so that `"player"_sh` compiles to a compile-time constant `StringHash`. |
| CORE-16 | `StringHash::of(std::string_view)` SHALL compute the hash at runtime. |
| CORE-17 | `consteval StringHash::of(const char* literal)` SHALL compute the hash at compile time when the input is a string literal. |
| CORE-18 | Two equal strings SHALL always produce the same `StringHash`. |
| CORE-19 | `StringHash` SHALL be equality-comparable and hashable (provide `std::hash<StringHash>` specialisation). |
| CORE-20 | A `StringHashRegistry` (optional, debug-only) SHALL store the original string alongside the hash to aid debugging. It SHALL be a no-op in Release builds. |

### 4.4 Result Type (`Result.h`)

| ID | Requirement |
|----|-------------|
| CORE-21 | `Result<T, E>` SHALL be a type alias for `std::expected<T, E>` (C++23). |
| CORE-22 | If the compiler does not support `std::expected`, a compatible polyfill SHALL be provided (e.g., `tl::expected`). |
| CORE-23 | `Err(e)` SHALL be a helper function returning `std::unexpected<E>` for use in error return sites. |
| CORE-24 | Usage SHALL compile without warnings at `-Wall -Wextra`. |

### 4.5 Assertions (`Assert.h`)

| ID | Requirement |
|----|-------------|
| CORE-25 | `EMBER_ASSERT(condition, message, ...)` SHALL, when `condition` is false: print the message (with file, line, function context) to the `ember` log channel at `ERROR` level, then trigger a debug break (`__debugbreak()` / `__builtin_trap()`) in Debug builds. |
| CORE-26 | In Release builds, `EMBER_ASSERT` SHALL be a no-op (zero runtime cost). |
| CORE-27 | `EMBER_VERIFY(condition, message, ...)` SHALL behave identically to `EMBER_ASSERT` in Debug builds. In Release builds it SHALL log the error and call `std::terminate()` (it is never a no-op). |
| CORE-28 | Both macros SHALL accept `printf`-style variadic format arguments via the logging system. |
| CORE-29 | Both macros SHALL expand to a single statement (safe to use in `if` without braces). |
| CORE-30 | The assertion handler SHALL be replaceable via `Assert::setHandler(fn)` for unit testing (allows tests to catch expected assertion failures). |

### 4.6 Logging (`Log.h`)

| ID | Requirement |
|----|-------------|
| CORE-31 | The logging system SHALL wrap spdlog and expose two named loggers: `"ember"` (engine internals) and `"game"` (game/script code). |
| CORE-32 | The following macros SHALL be provided for each channel: |
|         | Engine: `EMBER_LOG_TRACE`, `EMBER_LOG_DEBUG`, `EMBER_LOG_INFO`, `EMBER_LOG_WARN`, `EMBER_LOG_ERROR`, `EMBER_LOG_FATAL` |
|         | Game:   `GAME_LOG_TRACE`, `GAME_LOG_DEBUG`, `GAME_LOG_INFO`, `GAME_LOG_WARN`, `GAME_LOG_ERROR`, `GAME_LOG_FATAL` |
| CORE-33 | All macros SHALL accept `std::format`-compatible format strings and variadic arguments. |
| CORE-34 | `EMBER_LOG_FATAL` and `GAME_LOG_FATAL` SHALL log the message then call `EMBER_VERIFY(false, ...)` — i.e., terminate in all builds. |
| CORE-35 | The log system SHALL write to two sinks simultaneously: a coloured console sink and a rotating file sink (`logs/ember.log`, max 5MB, 3 rotations). |
| CORE-36 | Log timestamps SHALL be formatted as `HH:MM:SS.mmm`. |
| CORE-37 | The minimum log level SHALL be configurable at runtime via `Log::setLevel(channel, level)`. |
| CORE-38 | In Release builds, `EMBER_LOG_TRACE` and `EMBER_LOG_DEBUG` SHALL be compiled out (zero runtime cost). |
| CORE-39 | `Log::init()` SHALL be idempotent — calling it more than once SHALL be safe. |
| CORE-40 | `Log::shutdown()` SHALL flush all pending log entries and close file sinks cleanly. |

### 4.7 Arena Allocator (`Arena.h`)

| ID | Requirement |
|----|-------------|
| CORE-41 | `ArenaAllocator` SHALL be constructed with a capacity in bytes: `ArenaAllocator arena(size_bytes)`. |
| CORE-42 | `arena.alloc<T>(args...)` SHALL placement-new a `T` in the arena's buffer and return a `T*`. |
| CORE-43 | `arena.allocRaw(bytes, alignment)` SHALL return a `void*` to aligned memory within the buffer. |
| CORE-44 | If the arena is full, `alloc` SHALL call `EMBER_ASSERT` and return `nullptr`. It SHALL NOT call `::operator new`. |
| CORE-45 | `arena.reset()` SHALL reset the bump pointer to the start of the buffer without calling any destructors. Callers are responsible for ensuring no live objects remain. |
| CORE-46 | `arena.used()` SHALL return the number of bytes currently allocated. |
| CORE-47 | `arena.capacity()` SHALL return the total buffer size. |
| CORE-48 | `ArenaAllocator` SHALL NOT be copyable. It MAY be movable. |
| CORE-49 | The backing buffer SHALL be heap-allocated (`std::make_unique<std::byte[]>`) so the arena itself can be stack-allocated. |
| CORE-50 | Default alignment SHALL be `alignof(std::max_align_t)`. |

### 4.8 Pool Allocator (`Pool.h`)

| ID | Requirement |
|----|-------------|
| CORE-51 | `PoolAllocator<T>` SHALL be constructed with a capacity: `PoolAllocator<Transform> pool(1024)`. |
| CORE-52 | `pool.alloc(args...)` SHALL return a `T*` to a new object constructed in the pool. |
| CORE-53 | `pool.free(ptr)` SHALL return the slot to the free list without calling `::operator delete`. |
| CORE-54 | `pool.alloc()` on a full pool SHALL call `EMBER_ASSERT` and return `nullptr`. |
| CORE-55 | Each slot SHALL be exactly `sizeof(T)` bytes, aligned to `alignof(T)`. |
| CORE-56 | `pool.size()` SHALL return the number of live objects. `pool.capacity()` the total slots. |

---

## 5. Platform Library Requirements (`engine/platform/`)

### 5.1 Platform Detection (`Platform.h`)

| ID | Requirement |
|----|-------------|
| PLAT-01 | The following macros SHALL be defined based on the target platform (set by `cmake/Platform.cmake`): `EMBER_PLATFORM_WINDOWS`, `EMBER_PLATFORM_LINUX`, `EMBER_PLATFORM_MACOS`. |
| PLAT-02 | Exactly one of the above SHALL be defined per build. |
| PLAT-03 | GLFW and any other platform-specific headers SHALL NOT appear in any public Ember header. All platform types SHALL be forward-declared or abstracted. |

### 5.2 Window (`Window.h`)

| ID | Requirement |
|----|-------------|
| PLAT-04 | `WindowSpec` SHALL be a plain struct with fields: `std::string title`, `u32 width`, `u32 height`, `bool vsync`, `bool fullscreen`. Default values: title = `"Ember"`, width = `1280`, height = `720`, vsync = `true`, fullscreen = `false`. |
| PLAT-05 | `Window` SHALL be constructible from a `WindowSpec`. Construction SHALL create the OS window and an OpenGL 4.6 core profile context (falling back to 4.1 on macOS). |
| PLAT-06 | Construction SHALL throw `std::runtime_error` (or return an error via `Result`) if window or context creation fails. |
| PLAT-07 | `window.pollEvents()` SHALL process all pending OS events, invoking the event callback for each. |
| PLAT-08 | `window.swapBuffers()` SHALL present the back buffer. |
| PLAT-09 | `window.shouldClose()` SHALL return `true` when the OS close button has been pressed. |
| PLAT-10 | `window.setEventCallback(fn)` SHALL register a `std::function<void(Event&)>` called for every platform event. Only one callback SHALL be active at a time. |
| PLAT-11 | `window.getWidth()` and `window.getHeight()` SHALL return the current framebuffer dimensions in pixels (accounting for HiDPI scaling). |
| PLAT-12 | `window.setVSync(bool)` SHALL enable or disable vsync at runtime. |
| PLAT-13 | `Window` SHALL NOT be copyable or movable (RAII, owns the OS resource). |
| PLAT-14 | The `Window` destructor SHALL cleanly destroy the GLFW window and context. |

### 5.3 Events (`Event.h`)

The event types below are the minimum required for this epic. The full event system (EventBus) is deferred to Epic 03.

| ID | Requirement |
|----|-------------|
| PLAT-15 | An `Event` base class SHALL be defined with a virtual `getType()` returning an `EventType` enum value. |
| PLAT-16 | The following concrete event types SHALL be defined in this epic: `WindowResizeEvent { u32 width, height }`, `WindowCloseEvent {}`. |
| PLAT-17 | The sandbox SHALL demonstrate receiving `WindowResizeEvent` and updating the OpenGL viewport accordingly. |

### 5.4 Timer (`Timer.h`)

| ID | Requirement |
|----|-------------|
| PLAT-18 | `Timer` SHALL provide a `reset()` method and an `elapsed()` method returning seconds as `f64`. |
| PLAT-19 | `elapsed()` SHALL use `std::chrono::high_resolution_clock` and return time since construction or last `reset()` call. |
| PLAT-20 | Resolution SHALL be sufficient to measure a single frame at 144fps (≤ 7ms frame time) without truncation. |
| PLAT-21 | A free function `Time::deltaTime()` SHALL return the duration of the previous frame in seconds as `f32`. It SHALL be updated once per frame by the main loop. |
| PLAT-22 | `Time::elapsed()` SHALL return total application run time in seconds as `f64`. |

### 5.5 FileSystem (`FileSystem.h`)

| ID | Requirement |
|----|-------------|
| PLAT-23 | `FileSystem::readTextFile(path)` SHALL return `Result<std::string, std::string>` — the file contents on success, an error message on failure. |
| PLAT-24 | `FileSystem::readBinaryFile(path)` SHALL return `Result<std::vector<std::byte>, std::string>`. |
| PLAT-25 | `FileSystem::writeTextFile(path, content)` SHALL return `Result<void, std::string>`. |
| PLAT-26 | `FileSystem::exists(path)` SHALL return `bool`. |
| PLAT-27 | `FileSystem::setAssetsRoot(path)` SHALL set the base directory for asset path resolution. |
| PLAT-28 | `FileSystem::resolvePath(virtualPath)` SHALL prepend the assets root to the virtual path and return a `std::filesystem::path`. |
| PLAT-29 | All path arguments SHALL accept `std::string_view` and `std::filesystem::path`. |

---

## 6. RHI Requirements (`engine/renderer/api/`)

### 6.1 Abstraction Design

| ID | Requirement |
|----|-------------|
| RHI-01 | The RHI SHALL be defined as a set of abstract base classes (interfaces) in `engine/renderer/api/include/`. |
| RHI-02 | The OpenGL implementation SHALL live in `engine/renderer/api/opengl/` and SHALL NOT be included by any code outside that directory except through the `RHI::init()` factory. |
| RHI-03 | No OpenGL header (`glad/gl.h`, `GL/gl.h`, etc.) SHALL be included in any public RHI header. OpenGL types SHALL NOT appear in the public API. |

### 6.2 Interfaces

| ID | Requirement |
|----|-------------|
| RHI-04 | `IVertexBuffer` SHALL declare: `bind()`, `unbind()`, `setData(const void* data, u32 size)`, `getSize() → u32`. |
| RHI-05 | `IIndexBuffer` SHALL declare: `bind()`, `unbind()`, `getCount() → u32`. |
| RHI-06 | `IShader` SHALL declare: `bind()`, `unbind()`, `setInt(name, value)`, `setFloat(name, value)`, `setVec2/3/4(name, value)`, `setMat3/4(name, value)`. |
| RHI-07 | `ITexture2D` SHALL declare: `bind(slot)`, `unbind()`, `getWidth() → u32`, `getHeight() → u32`. |
| RHI-08 | `IFramebuffer` SHALL declare: `bind()`, `unbind()`, `resize(w, h)`, `getColorAttachment() → u32` (returns texture ID for display). |
| RHI-09 | All interface destructors SHALL be `virtual`. All resources SHALL clean up GPU state in their destructor. |

### 6.3 Factory & Initialization

| ID | Requirement |
|----|-------------|
| RHI-10 | `RHI::init(Backend backend)` SHALL initialize the selected backend. In this epic, `Backend::OpenGL` is the only valid value. Passing an unsupported value SHALL call `EMBER_VERIFY`. |
| RHI-11 | `RHI::createVertexBuffer(const void* data, u32 size, BufferUsage usage)` SHALL return a `std::shared_ptr<IVertexBuffer>`. |
| RHI-12 | `RHI::createIndexBuffer(const u32* indices, u32 count)` SHALL return a `std::shared_ptr<IIndexBuffer>`. |
| RHI-13 | `RHI::createShader(const std::string& vertSrc, const std::string& fragSrc)` SHALL return a `std::shared_ptr<IShader>`. Compilation errors SHALL be logged and return `nullptr`. |
| RHI-14 | `RHI::createTexture2D(const TextureSpec& spec)` SHALL return a `std::shared_ptr<ITexture2D>`. |

### 6.4 Draw Commands

| ID | Requirement |
|----|-------------|
| RHI-15 | `RHI::setViewport(x, y, width, height)` SHALL set the GPU viewport. |
| RHI-16 | `RHI::setClearColor(r, g, b, a)` SHALL set the clear colour. |
| RHI-17 | `RHI::clear()` SHALL clear the colour and depth buffers. |
| RHI-18 | `RHI::drawIndexed(vertexBuffer, indexBuffer, indexCount)` SHALL issue a single indexed draw call. |

### 6.5 OpenGL Backend Specifics

| ID | Requirement |
|----|-------------|
| RHI-19 | The OpenGL context SHALL be created as a **core profile** context, version 4.6 on Windows/Linux and 4.1 on macOS. |
| RHI-20 | `glad2` SHALL be used for OpenGL function loading. `gladLoadGL` SHALL be called after context creation and before any GL calls. |
| RHI-21 | In Debug builds, OpenGL debug output SHALL be enabled (`glDebugMessageCallback`) and all messages of severity `GL_DEBUG_SEVERITY_HIGH` or `GL_DEBUG_SEVERITY_MEDIUM` SHALL be forwarded to `EMBER_LOG_ERROR`. |
| RHI-22 | In Release builds, the debug callback SHALL not be registered. |
| RHI-23 | `OpenGLShader` SHALL compile vertex and fragment shaders separately, link them, then check for errors. Shader source SHALL be loadable from strings (for this epic; file loading is added in Epic 04). |

---

## 7. Sandbox Requirements (`sandbox/`)

| ID | Requirement |
|----|-------------|
| SBX-01 | The sandbox SHALL be an executable target `ember_sandbox` that links against `ember`. |
| SBX-02 | The sandbox `main()` SHALL initialise the logging system, create a `Window` with the default `WindowSpec`, and enter the main loop. |
| SBX-03 | The main loop SHALL: call `window.pollEvents()`, call `RHI::clear()`, submit a draw call for a hard-coded white triangle, call `window.swapBuffers()`. |
| SBX-04 | The triangle SHALL be defined by a hard-coded vertex array (no asset loading). It SHALL be visible, centred, and fill roughly a third of the window height. |
| SBX-05 | The window title SHALL display the current FPS, updated once per second (e.g., `"Ember Sandbox — 144 FPS"`). |
| SBX-06 | Pressing `Escape` or closing the window SHALL cleanly exit the main loop and terminate the process with exit code `0`. |
| SBX-07 | On a `WindowResizeEvent`, the sandbox SHALL call `RHI::setViewport` with the new dimensions. The triangle SHALL remain centred after resize. |
| SBX-08 | The sandbox SHALL log "Ember initialised" at `INFO` level before the main loop begins, and "Ember shutting down" at `INFO` level after the loop exits. |

---

## 8. Test Requirements (`tests/`)

### 8.1 Framework

| ID | Requirement |
|----|-------------|
| TST-01 | Catch2 version 3.x SHALL be the test framework. |
| TST-02 | Tests SHALL be compiled into an executable target `ember_tests`. |
| TST-03 | `ember_tests` SHALL be registered with CTest via `catch_discover_tests`. |
| TST-04 | Running `ctest --preset release` SHALL execute all tests and report pass/fail. |

### 8.2 Required Test Cases

Each test case maps to a specific requirement above.

#### Core — UUID
| Test | Verifies |
|------|----------|
| `UUID_NeverReturnsNull` | 1 000 000 consecutive calls to `generateUUID()` produce no `0` value | CORE-10 |
| `UUID_NoDuplicatesInMillion` | All 1 000 000 values are unique (stored in `std::unordered_set`) | CORE-11 |
| `UUID_IsThreadSafe` | 4 threads each call `generateUUID()` 250 000 times; no duplicates across all threads | CORE-12 |

#### Core — StringHash
| Test | Verifies |
|------|----------|
| `StringHash_CompileTimeEqualsRuntime` | `"player"_sh == StringHash::of("player")` | CORE-15/16 |
| `StringHash_SameStringSameHash` | Two runtime calls with identical input return equal hashes | CORE-18 |
| `StringHash_DifferentStrings` | `StringHash::of("player") != StringHash::of("enemy")` (collision check on 100 known strings) | CORE-18 |
| `StringHash_IsConstexpr` | `constexpr auto h = "test"_sh;` compiles | CORE-15 |
| `StringHash_HashableInUnorderedMap` | `std::unordered_map<StringHash, int>` compiles and inserts/finds correctly | CORE-19 |

#### Core — ArenaAllocator
| Test | Verifies |
|------|----------|
| `Arena_AllocAndUse` | Allocate a struct, write fields, read back correctly | CORE-42 |
| `Arena_ResetAndReuse` | Allocate to capacity, reset, re-allocate same capacity without error | CORE-45 |
| `Arena_FullArena` | Allocating past capacity triggers assertion (tested via `Assert::setHandler`) | CORE-44 |
| `Arena_AlignmentCorrect` | `allocRaw(1, 16)` returns a 16-byte aligned pointer | CORE-43 |
| `Arena_UsedAndCapacity` | `used()` increases correctly per allocation; `capacity()` is constant | CORE-46/47 |

#### Core — PoolAllocator
| Test | Verifies |
|------|----------|
| `Pool_AllocFreeAlloc` | Allocate 3 objects, free the middle one, allocate again — slot is reused | CORE-53 |
| `Pool_FullPool` | Filling pool to capacity and allocating one more triggers assertion | CORE-54 |
| `Pool_SizeTracking` | `size()` correctly tracks live object count through alloc/free operations | CORE-56 |

#### Core — Logging
| Test | Verifies |
|------|----------|
| `Log_InitIdempotent` | `Log::init()` called 3 times in a row does not crash or duplicate sinks | CORE-39 |
| `Log_ShutdownFlushes` | After `EMBER_LOG_INFO("test")` and `Log::shutdown()`, the log file contains the message | CORE-40 |
| `Log_LevelFilter` | Setting level to `WARN` causes `INFO` messages to not appear in the file sink | CORE-37 |

#### Platform — Timer
| Test | Verifies |
|------|----------|
| `Timer_ElapsedIncreases` | `elapsed()` called twice returns a larger value the second time | PLAT-18 |
| `Timer_ResetResetsElapsed` | After `reset()`, `elapsed()` is near zero | PLAT-18 |
| `Timer_SubMillisecondResolution` | Sleeping 1ms and measuring gives a result between 0.5ms and 5ms (loose bounds for CI) | PLAT-20 |

#### Platform — FileSystem
| Test | Verifies |
|------|----------|
| `FS_ReadWriteRoundtrip` | Write a temp text file, read it back, contents match | PLAT-23/25 |
| `FS_ExistsReturnsFalse` | `exists()` returns `false` for a path that does not exist | PLAT-26 |
| `FS_ReadNonexistentReturnsError` | `readTextFile` on a missing path returns an `Err` result | PLAT-23 |
| `FS_VirtualPathResolution` | After `setAssetsRoot("/tmp/assets")`, `resolvePath("textures/a.png")` returns `"/tmp/assets/textures/a.png"` | PLAT-28 |

---

## 9. Non-Functional Requirements

| ID | Requirement |
|----|-------------|
| NFR-01 | **Cold build time.** A full clean build from `cmake --build --preset release` SHALL complete in under 5 minutes on a modern 8-core developer machine. |
| NFR-02 | **Incremental build time.** Modifying a single `.cpp` file in `engine/core/` and rebuilding SHALL complete in under 10 seconds. |
| NFR-03 | **Runtime memory.** The sandbox with one triangle on screen SHALL use less than 50MB of resident memory. |
| NFR-04 | **Triangle frame rate.** The sandbox SHALL sustain ≥ 60fps on integrated graphics (Intel Iris Xe or equivalent) with vsync off. |
| NFR-05 | **No UB.** Running the sandbox under ASan + UBSan (`sanitize` preset) SHALL produce zero error reports at startup and after a 10-second run. |
| NFR-06 | **No memory leaks.** Running the sandbox under ASan SHALL produce a clean leak report on shutdown. |
| NFR-07 | **Compile-time safety.** All type aliases, assertions, and StringHash constexpr functions SHALL produce compile-time errors if misused (e.g., assigning `u32` where `u64` is required without explicit cast). |
| NFR-08 | **Header hygiene.** Including any single public Ember header SHALL NOT pull in `<windows.h>`, `<X11/Xlib.h>`, GLFW, or any other platform header. |
| NFR-09 | **Include cost.** `#include "ember/core/Types.h"` SHALL add no more than 50ms to a fresh compilation of an otherwise empty translation unit (measured with `-ftime-report` or equivalent). |

---

## 10. Out of Scope

The following are explicitly deferred to later epics and SHALL NOT be implemented in this epic:

- ECS, components, or any game object system
- Scene loading or serialization
- Asset loading (textures, fonts, audio)
- Full event bus (only stub `Event` base class and two window event types)
- Input system
- ImGui or any editor UI
- 2D sprite rendering or batching
- Audio
- Scripting

---

## 11. Acceptance Criteria Summary

The epic is complete when ALL of the following are true:

1. `cmake --preset release && cmake --build --preset release` exits with code `0` on a fresh clone on Windows, Linux, and macOS.
2. `ctest --preset release` reports `N tests passed, 0 tests failed` where N ≥ 20.
3. The `ember_sandbox` executable opens a window and renders a white triangle at ≥ 60fps.
4. The triangle remains correctly centred after resizing the window.
5. Closing the window exits the process with code `0` and writes `"Ember shutting down"` to the log.
6. Running `ember_sandbox` under the `sanitize` preset produces no ASan/UBSan errors.
7. The GitHub Actions CI workflow is green on the `main` branch for all three platform jobs.
8. No compiler warning is emitted on any of the three platforms.
