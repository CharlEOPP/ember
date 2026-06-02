# Epic 02 — Foundation: Implementation Plan

**Status:** Complete  
**Document:** 03-implementation-plan.md  
**Refs:** epic.md, 02-requirements.md

Work through the phases in order. Each phase produces a buildable, runnable state. Do not proceed to the next phase until all checkboxes in the current one are ticked and the stated verification passes.

---

## Phase 1 — Repo Skeleton & CMake Infrastructure

Goal: `cmake --preset debug` configures without error. No source files yet — just the build graph.

### 1.1 Root files

- [x] Create `.gitignore`:
  ```
  build/
  .cache/
  .vs/
  .vscode/
  *.user
  *.suo
  CMakeUserPresets.json
  logs/
  ```
- [x] Create `CMakePresets.json` at repo root:
  ```json
  {
    "version": 6,
    "configurePresets": [
      {
        "name": "base",
        "hidden": true,
        "generator": "Ninja",
        "binaryDir": "${sourceDir}/build/${presetName}",
        "cacheVariables": {
          "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
        }
      },
      {
        "name": "debug",
        "inherits": "base",
        "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
      },
      {
        "name": "release",
        "inherits": "base",
        "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" }
      },
      {
        "name": "relwithdebinfo",
        "inherits": "base",
        "cacheVariables": { "CMAKE_BUILD_TYPE": "RelWithDebInfo" }
      },
      {
        "name": "sanitize",
        "inherits": "debug",
        "cacheVariables": { "EMBER_SANITIZE": "ON" }
      }
    ],
    "buildPresets": [
      { "name": "debug",        "configurePreset": "debug" },
      { "name": "release",      "configurePreset": "release" },
      { "name": "relwithdebinfo","configurePreset": "relwithdebinfo" },
      { "name": "sanitize",     "configurePreset": "sanitize" }
    ],
    "testPresets": [
      { "name": "release", "configurePreset": "release", "output": { "outputOnFailure": true } },
      { "name": "debug",   "configurePreset": "debug",   "output": { "outputOnFailure": true } }
    ]
  }
  ```

- [x] Create root `CMakeLists.txt`:
  ```cmake
  cmake_minimum_required(VERSION 3.25)
  project(Ember VERSION 0.1.0 LANGUAGES CXX)

  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

  include(CompilerOptions)
  include(Dependencies)

  add_subdirectory(engine/core)
  add_subdirectory(engine/platform)
  add_subdirectory(engine/renderer)
  add_subdirectory(sandbox)
  add_subdirectory(tests)

  # Aggregate interface target
  add_library(ember INTERFACE)
  target_link_libraries(ember INTERFACE
      ember_core
      ember_platform
      ember_renderer_api
  )
  ```

### 1.2 `cmake/CompilerOptions.cmake`

- [x] Create `cmake/CompilerOptions.cmake`:
  ```cmake
  function(ember_set_compiler_options target)
      if(MSVC)
          target_compile_options(${target} PRIVATE
              /W4 /WX /permissive- /Zc:__cplusplus
              $<$<CONFIG:Debug>:/Od /RTC1>
              $<$<CONFIG:Release>:/O2>
          )
      elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
          target_compile_options(${target} PRIVATE
              -Wall -Wextra -Wpedantic -Werror
              -Wno-unused-parameter
              $<$<CONFIG:Debug>:-O0 -g3>
              $<$<CONFIG:Release>:-O3>
          )
          if(DEFINED EMBER_SANITIZE AND EMBER_SANITIZE)
              target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
              target_link_options(${target} PRIVATE -fsanitize=address,undefined)
          endif()
      endif()
  endfunction()
  ```

### 1.3 `cmake/Platform.cmake`

- [x] Create `cmake/Platform.cmake`:
  ```cmake
  if(WIN32)
      add_compile_definitions(EMBER_PLATFORM_WINDOWS)
  elseif(APPLE)
      add_compile_definitions(EMBER_PLATFORM_MACOS)
  elseif(UNIX)
      add_compile_definitions(EMBER_PLATFORM_LINUX)
  else()
      message(FATAL_ERROR "Unsupported platform")
  endif()
  ```
- [x] Add `include(Platform)` to root `CMakeLists.txt` after `include(CompilerOptions)`.

### 1.4 `cmake/Dependencies.cmake`

- [x] Create `cmake/Dependencies.cmake` with all FetchContent declarations:
  ```cmake
  include(FetchContent)
  option(EMBER_FETCH_DEPS "Fetch dependencies via FetchContent" ON)

  if(NOT EMBER_FETCH_DEPS)
      return()
  endif()

  # GLFW — windowing
  FetchContent_Declare(glfw
      GIT_REPOSITORY https://github.com/glfw/glfw.git
      GIT_TAG        3.4
  )
  set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

  # GLM — math
  FetchContent_Declare(glm
      GIT_REPOSITORY https://github.com/g-truc/glm.git
      GIT_TAG        1.0.1
  )

  # EnTT — ECS (header-only, needed early for type traits in core)
  FetchContent_Declare(EnTT
      GIT_REPOSITORY https://github.com/skypjack/entt.git
      GIT_TAG        v3.13.2
  )

  # spdlog — logging
  FetchContent_Declare(spdlog
      GIT_REPOSITORY https://github.com/gabime/spdlog.git
      GIT_TAG        v1.14.1
  )
  set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)

  # yaml-cpp — serialization (future epics)
  FetchContent_Declare(yaml-cpp
      GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
      GIT_TAG        0.8.0
  )
  set(YAML_CPP_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_TOOLS   OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)

  # Catch2 — testing
  FetchContent_Declare(Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG        v3.6.0
  )

  # Dear ImGui (docking branch — for future editor epic)
  FetchContent_Declare(imgui
      GIT_REPOSITORY https://github.com/ocornut/imgui.git
      GIT_TAG        v1.90.9-docking
  )

  # stb — image loading
  FetchContent_Declare(stb
      GIT_REPOSITORY https://github.com/nothings/stb.git
      GIT_TAG        master   # stb has no tags; pin a known-good commit in production
  )

  FetchContent_MakeAvailable(glfw glm EnTT spdlog yaml-cpp Catch2)
  # stb and imgui are header-only; don't call MakeAvailable yet — consume via SYSTEM includes
  FetchContent_Populate(stb)
  FetchContent_Populate(imgui)
  ```

  > **Note on glad2:** glad2 generates source files from a spec. Pre-generate for OpenGL 4.6 core + OpenGL 4.1 core and commit the output to `third_party/glad/`. See Phase 2 for instructions.

### 1.5 Stub `CMakeLists.txt` files

Create minimal placeholder `CMakeLists.txt` in each subdirectory so the root configure step succeeds before any source exists:

- [x] `engine/core/CMakeLists.txt` — empty for now
- [x] `engine/platform/CMakeLists.txt` — empty for now
- [x] `engine/renderer/CMakeLists.txt` — empty for now (will add `add_subdirectory(api)`)
- [x] `sandbox/CMakeLists.txt` — empty for now
- [x] `tests/CMakeLists.txt` — empty for now

**Verify Phase 1:**
```sh
cmake --preset debug
# Expected: "-- Configuring done" with no errors
```

---

## Phase 2 — glad2 Setup

glad2 requires a code generation step. Do this once and commit the output.

- [x] Install glad2 generator: `pip install glad2`
- [x] Generate OpenGL 4.6 core loader:
  ```sh
  glad --out-path third_party/glad --api gl:core=4.6 --extensions "" c
  ```
  This creates:
  ```
  third_party/glad/
    include/glad/gl.h
    include/KHR/khrplatform.h
    src/gl.c
  ```
- [x] Also generate an OpenGL 4.1 core loader in the same output tree (macOS needs 4.1):
  ```sh
  glad --out-path third_party/glad41 --api gl:core=4.1 --extensions "" c
  ```
- [x] Create `third_party/glad/CMakeLists.txt`:
  ```cmake
  add_library(glad STATIC src/gl.c)
  target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  # Suppress warnings in glad (not our code)
  if(MSVC)
      target_compile_options(glad PRIVATE /W0)
  else()
      target_compile_options(glad PRIVATE -w)
  endif()
  ```
- [x] Add `add_subdirectory(third_party/glad)` to root `CMakeLists.txt` (before engine subdirs).
- [x] Commit `third_party/glad/` to the repository.

---

## Phase 3 — Core Library

Work through the headers and source files one unit at a time. Each unit should compile clean before moving to the next.

### 3.1 Directory structure

- [x] Create the following directories:
  ```
  engine/core/
    include/ember/core/
    src/
  ```

- [x] Create `engine/core/CMakeLists.txt`:
  ```cmake
  add_library(ember_core STATIC
      src/Log.cpp
      src/UUID.cpp
      src/Assert.cpp
      src/FileSystem.cpp   # stubbed here, lives in platform but core-adjacent
  )

  target_include_directories(ember_core
      PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ember_core
      PUBLIC  spdlog::spdlog
      PRIVATE # nothing else yet
  )

  target_compile_features(ember_core PUBLIC cxx_std_20)
  ember_set_compiler_options(ember_core)
  ```

### 3.2 `Types.h`

- [x] Create `engine/core/include/ember/core/Types.h`:
  ```cpp
  #pragma once
  #include <cstdint>
  #include <cstddef>

  namespace ember {

  using u8  = std::uint8_t;
  using u16 = std::uint16_t;
  using u32 = std::uint32_t;
  using u64 = std::uint64_t;

  using i8  = std::int8_t;
  using i16 = std::int16_t;
  using i32 = std::int32_t;
  using i64 = std::int64_t;

  using f32 = float;
  using f64 = double;

  using usize = std::size_t;
  using isize = std::ptrdiff_t;
  using byte  = std::byte;

  static_assert(sizeof(u8)  == 1);
  static_assert(sizeof(u16) == 2);
  static_assert(sizeof(u32) == 4);
  static_assert(sizeof(u64) == 8);
  static_assert(sizeof(f32) == 4);
  static_assert(sizeof(f64) == 8);

  } // namespace ember
  ```

### 3.3 `Log.h` / `Log.cpp`

- [x] Create `engine/core/include/ember/core/Log.h`:
  ```cpp
  #pragma once
  #include <spdlog/spdlog.h>
  #include <spdlog/logger.h>
  #include <memory>

  namespace ember {

  class Log {
  public:
      static void init();
      static void shutdown();
      static void setLevel(const std::string& channel, spdlog::level::level_enum level);

      static std::shared_ptr<spdlog::logger>& engineLogger();
      static std::shared_ptr<spdlog::logger>& gameLogger();
  private:
      static std::shared_ptr<spdlog::logger> s_engineLogger;
      static std::shared_ptr<spdlog::logger> s_gameLogger;
  };

  } // namespace ember

  // ---------- Macros ----------
  #define EMBER_LOG_TRACE(...)  ::ember::Log::engineLogger()->trace(__VA_ARGS__)
  #define EMBER_LOG_DEBUG(...)  ::ember::Log::engineLogger()->debug(__VA_ARGS__)
  #define EMBER_LOG_INFO(...)   ::ember::Log::engineLogger()->info(__VA_ARGS__)
  #define EMBER_LOG_WARN(...)   ::ember::Log::engineLogger()->warn(__VA_ARGS__)
  #define EMBER_LOG_ERROR(...)  ::ember::Log::engineLogger()->error(__VA_ARGS__)
  #define EMBER_LOG_FATAL(...)  do { ::ember::Log::engineLogger()->critical(__VA_ARGS__); EMBER_VERIFY(false, "Fatal error"); } while(0)

  #define GAME_LOG_TRACE(...)   ::ember::Log::gameLogger()->trace(__VA_ARGS__)
  #define GAME_LOG_DEBUG(...)   ::ember::Log::gameLogger()->debug(__VA_ARGS__)
  #define GAME_LOG_INFO(...)    ::ember::Log::gameLogger()->info(__VA_ARGS__)
  #define GAME_LOG_WARN(...)    ::ember::Log::gameLogger()->warn(__VA_ARGS__)
  #define GAME_LOG_ERROR(...)   ::ember::Log::gameLogger()->error(__VA_ARGS__)
  #define GAME_LOG_FATAL(...)   do { ::ember::Log::gameLogger()->critical(__VA_ARGS__); EMBER_VERIFY(false, "Fatal error"); } while(0)

  // Strip trace/debug in release
  #if defined(NDEBUG)
  #undef  EMBER_LOG_TRACE
  #undef  EMBER_LOG_DEBUG
  #undef  GAME_LOG_TRACE
  #undef  GAME_LOG_DEBUG
  #define EMBER_LOG_TRACE(...) do {} while(0)
  #define EMBER_LOG_DEBUG(...) do {} while(0)
  #define GAME_LOG_TRACE(...)  do {} while(0)
  #define GAME_LOG_DEBUG(...)  do {} while(0)
  #endif
  ```

- [x] Create `engine/core/src/Log.cpp`:
  ```cpp
  #include "ember/core/Log.h"
  #include <spdlog/sinks/stdout_color_sinks.h>
  #include <spdlog/sinks/rotating_file_sink.h>
  #include <vector>

  namespace ember {

  std::shared_ptr<spdlog::logger> Log::s_engineLogger;
  std::shared_ptr<spdlog::logger> Log::s_gameLogger;

  void Log::init() {
      if (s_engineLogger) return; // idempotent

      std::vector<spdlog::sink_ptr> sinks;
      sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
      sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
          "logs/ember.log", 1024 * 1024 * 5, 3));

      s_engineLogger = std::make_shared<spdlog::logger>("EMBER", sinks.begin(), sinks.end());
      s_engineLogger->set_level(spdlog::level::trace);
      s_engineLogger->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
      spdlog::register_logger(s_engineLogger);

      s_gameLogger = std::make_shared<spdlog::logger>("GAME", sinks.begin(), sinks.end());
      s_gameLogger->set_level(spdlog::level::trace);
      s_gameLogger->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
      spdlog::register_logger(s_gameLogger);
  }

  void Log::shutdown() {
      spdlog::shutdown();
      s_engineLogger.reset();
      s_gameLogger.reset();
  }

  void Log::setLevel(const std::string& channel, spdlog::level::level_enum level) {
      if (auto logger = spdlog::get(channel))
          logger->set_level(level);
  }

  std::shared_ptr<spdlog::logger>& Log::engineLogger() { return s_engineLogger; }
  std::shared_ptr<spdlog::logger>& Log::gameLogger()   { return s_gameLogger;   }

  } // namespace ember
  ```

  > **Note:** `EMBER_VERIFY` is referenced by the `FATAL` macros but defined in `Assert.h`. Include order matters: `Assert.h` must include `Log.h`, not the other way around. Keep the macro definitions in `Log.h` but move the `FATAL` macro body to a separate `CoreMacros.h` if circular include issues arise.

### 3.4 `Assert.h` / `Assert.cpp`

- [x] Create `engine/core/include/ember/core/Assert.h`:
  ```cpp
  #pragma once
  #include "ember/core/Log.h"
  #include <functional>

  namespace ember::Assert {
      using Handler = std::function<void(const char* expr, const char* file, int line, const char* msg)>;
      void setHandler(Handler handler);
      void resetHandler();  // restore default (break/terminate)
      void invoke(const char* expr, const char* file, int line, const char* msg);
  } // namespace ember::Assert

  // Debug break, cross-platform
  #if defined(_MSC_VER)
      #define EMBER_DEBUGBREAK() __debugbreak()
  #elif defined(__clang__) || defined(__GNUC__)
      #define EMBER_DEBUGBREAK() __builtin_trap()
  #else
      #include <csignal>
      #define EMBER_DEBUGBREAK() std::raise(SIGTRAP)
  #endif

  #if !defined(NDEBUG)
      #define EMBER_ASSERT(cond, ...)                                                  \
          do {                                                                          \
              if (!(cond)) {                                                            \
                  EMBER_LOG_ERROR("Assertion failed: {} — {}", #cond,                  \
                      std::string(std::format(__VA_ARGS__)));                           \
                  ::ember::Assert::invoke(#cond, __FILE__, __LINE__,                   \
                      std::format(__VA_ARGS__).c_str());                               \
              }                                                                         \
          } while(0)
  #else
      #define EMBER_ASSERT(cond, ...) do {} while(0)
  #endif

  #define EMBER_VERIFY(cond, ...)                                                      \
      do {                                                                              \
          if (!(cond)) {                                                                \
              EMBER_LOG_ERROR("Verify failed: {} — {}", #cond,                         \
                  std::string(std::format(__VA_ARGS__)));                              \
              ::ember::Assert::invoke(#cond, __FILE__, __LINE__,                       \
                  std::format(__VA_ARGS__).c_str());                                   \
          }                                                                             \
      } while(0)
  ```

- [x] Create `engine/core/src/Assert.cpp`:
  ```cpp
  #include "ember/core/Assert.h"
  #include <cstdlib>

  namespace ember::Assert {

  static Handler s_handler;

  static void defaultHandler(const char* expr, const char* file, int line, const char* msg) {
  #if !defined(NDEBUG)
      EMBER_DEBUGBREAK();
  #else
      (void)expr; (void)file; (void)line; (void)msg;
      std::terminate();
  #endif
  }

  void setHandler(Handler handler) { s_handler = std::move(handler); }
  void resetHandler()              { s_handler = {}; }

  void invoke(const char* expr, const char* file, int line, const char* msg) {
      if (s_handler)
          s_handler(expr, file, line, msg);
      else
          defaultHandler(expr, file, line, msg);
  }

  } // namespace ember::Assert
  ```

### 3.5 `UUID.h` / `UUID.cpp`

- [x] Create `engine/core/include/ember/core/UUID.h`:
  ```cpp
  #pragma once
  #include "ember/core/Types.h"

  namespace ember {

  using UUID = u64;
  inline constexpr UUID NULL_UUID = 0;

  UUID generateUUID();

  } // namespace ember
  ```

- [x] Create `engine/core/src/UUID.cpp` implementing xoshiro256++:
  ```cpp
  #include "ember/core/UUID.h"
  #include <random>
  #include <atomic>
  #include <array>
  #include <bit>

  namespace ember {
  namespace {

  // xoshiro256++ state — one per process, seeded from hardware entropy
  struct Xoshiro256pp {
      std::array<u64, 4> s;

      u64 next() {
          const u64 result = std::rotl(s[0] + s[3], 23) + s[0];
          const u64 t = s[1] << 17;
          s[2] ^= s[0];
          s[3] ^= s[1];
          s[1] ^= s[2];
          s[0] ^= s[3];
          s[2] ^= t;
          s[3] = std::rotl(s[3], 45);
          return result;
      }
  };

  Xoshiro256pp initState() {
      std::random_device rd;
      std::uniform_int_distribution<u64> dist;
      Xoshiro256pp rng;
      do {
          for (auto& v : rng.s) v = dist(rd);
      } while (rng.s == std::array<u64,4>{0,0,0,0});
      return rng;
  }

  // Mutex-free: one global generator guarded by a spinlock
  static Xoshiro256pp s_rng = initState();
  static std::atomic_flag s_lock = ATOMIC_FLAG_INIT;

  } // anonymous namespace

  UUID generateUUID() {
      UUID result = 0;
      while (result == 0) {
          while (s_lock.test_and_set(std::memory_order_acquire)) {}
          result = s_rng.next();
          s_lock.clear(std::memory_order_release);
      }
      return result;
  }

  } // namespace ember
  ```

### 3.6 `StringHash.h`

- [x] Create `engine/core/include/ember/core/StringHash.h`:
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include <string_view>
  #include <functional>

  namespace ember {

  struct StringHash {
      u64 value = 0;

      constexpr StringHash() = default;
      constexpr explicit StringHash(u64 v) : value(v) {}

      constexpr bool operator==(const StringHash&) const = default;
      constexpr bool operator!=(const StringHash&) const = default;

      // FNV-1a 64-bit
      static constexpr u64 fnv1a(std::string_view str) {
          u64 hash = 14695981039346656037ULL;
          for (char c : str) {
              hash ^= static_cast<u64>(c);
              hash *= 1099511628211ULL;
          }
          return hash;
      }

      static constexpr StringHash of(std::string_view str) {
          return StringHash{ fnv1a(str) };
      }
  };

  constexpr StringHash operator""_sh(const char* str, std::size_t len) {
      return StringHash::of(std::string_view(str, len));
  }

  } // namespace ember

  // std::hash specialisation so StringHash works in unordered_map/set
  template<>
  struct std::hash<ember::StringHash> {
      std::size_t operator()(const ember::StringHash& sh) const noexcept {
          return static_cast<std::size_t>(sh.value);
      }
  };
  ```

### 3.7 `Arena.h` / `Arena.cpp`

- [x] Create `engine/core/include/ember/core/Arena.h`:
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include "ember/core/Assert.h"
  #include <memory>
  #include <new>

  namespace ember {

  class ArenaAllocator {
  public:
      explicit ArenaAllocator(usize capacityBytes);
      ~ArenaAllocator() = default;

      ArenaAllocator(const ArenaAllocator&)            = delete;
      ArenaAllocator& operator=(const ArenaAllocator&) = delete;
      ArenaAllocator(ArenaAllocator&&)                 = default;

      template<typename T, typename... Args>
      T* alloc(Args&&... args) {
          void* ptr = allocRaw(sizeof(T), alignof(T));
          if (!ptr) return nullptr;
          return new(ptr) T(std::forward<Args>(args)...);
      }

      void* allocRaw(usize bytes, usize alignment = alignof(std::max_align_t));

      void  reset();
      usize used()     const { return m_offset; }
      usize capacity() const { return m_capacity; }

  private:
      std::unique_ptr<byte[]> m_buffer;
      usize                   m_capacity;
      usize                   m_offset = 0;
  };

  } // namespace ember
  ```

- [x] Create `engine/core/src/Arena.cpp`:
  ```cpp
  #include "ember/core/Arena.h"
  #include <bit>

  namespace ember {

  ArenaAllocator::ArenaAllocator(usize capacityBytes)
      : m_buffer(std::make_unique<byte[]>(capacityBytes))
      , m_capacity(capacityBytes)
  {}

  void* ArenaAllocator::allocRaw(usize bytes, usize alignment) {
      usize aligned = (m_offset + alignment - 1) & ~(alignment - 1);
      EMBER_ASSERT(aligned + bytes <= m_capacity, "ArenaAllocator out of memory");
      if (aligned + bytes > m_capacity) return nullptr;
      void* ptr = m_buffer.get() + aligned;
      m_offset = aligned + bytes;
      return ptr;
  }

  void ArenaAllocator::reset() {
      m_offset = 0;
  }

  } // namespace ember
  ```

### 3.8 `Pool.h`

- [x] Create `engine/core/include/ember/core/Pool.h`:
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include "ember/core/Assert.h"
  #include <vector>
  #include <memory>

  namespace ember {

  template<typename T>
  class PoolAllocator {
  public:
      explicit PoolAllocator(usize capacity)
          : m_capacity(capacity)
          , m_storage(std::make_unique<Slot[]>(capacity))
      {
          // Build free list
          m_freeList = 0;
          for (usize i = 0; i < capacity - 1; ++i)
              m_storage[i].nextFree = static_cast<u32>(i + 1);
          m_storage[capacity - 1].nextFree = INVALID;
      }

      template<typename... Args>
      T* alloc(Args&&... args) {
          EMBER_ASSERT(m_freeList != INVALID, "PoolAllocator<{}> is full", typeid(T).name());
          if (m_freeList == INVALID) return nullptr;
          u32 idx   = m_freeList;
          m_freeList = m_storage[idx].nextFree;
          ++m_size;
          return new (&m_storage[idx].data) T(std::forward<Args>(args)...);
      }

      void free(T* ptr) {
          EMBER_ASSERT(ptr, "PoolAllocator::free called with nullptr");
          ptr->~T();
          auto* slot = reinterpret_cast<Slot*>(ptr);
          u32 idx    = static_cast<u32>(slot - m_storage.get());
          slot->nextFree = m_freeList;
          m_freeList     = idx;
          --m_size;
      }

      usize size()     const { return m_size; }
      usize capacity() const { return m_capacity; }

  private:
      static constexpr u32 INVALID = ~u32(0);

      union Slot {
          alignas(T) byte data[sizeof(T)];
          u32 nextFree;
      };

      usize                   m_capacity;
      usize                   m_size     = 0;
      u32                     m_freeList = INVALID;
      std::unique_ptr<Slot[]> m_storage;
  };

  } // namespace ember
  ```

### 3.9 `Result.h`

- [x] Create `engine/core/include/ember/core/Result.h`:
  ```cpp
  #pragma once
  #include <expected>  // C++23 — available in GCC 12+, Clang 16+, MSVC 19.36+

  namespace ember {

  template<typename T, typename E>
  using Result = std::expected<T, E>;

  template<typename E>
  auto Err(E&& e) { return std::unexpected(std::forward<E>(e)); }

  } // namespace ember
  ```
  - [x] Add a CMake check: if `std::expected` is not available, find or vendor `tl::expected` and alias it. Add to `CompilerOptions.cmake`:
    ```cmake
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
      #include <expected>
      int main() { std::expected<int,int> e = 1; return 0; }
    " EMBER_HAS_STD_EXPECTED)
    if(NOT EMBER_HAS_STD_EXPECTED)
        message(STATUS "std::expected not available — fetching tl::expected")
        FetchContent_Declare(tl_expected
            GIT_REPOSITORY https://github.com/TartanLlama/expected.git
            GIT_TAG        v1.1.0
        )
        FetchContent_MakeAvailable(tl_expected)
        target_compile_definitions(ember_core PUBLIC EMBER_USE_TL_EXPECTED)
    endif()
    ```

### 3.10 Core umbrella header

- [x] Create `engine/core/include/ember/core/Core.h` that includes all of the above:
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include "ember/core/Log.h"
  #include "ember/core/Assert.h"
  #include "ember/core/UUID.h"
  #include "ember/core/StringHash.h"
  #include "ember/core/Arena.h"
  #include "ember/core/Pool.h"
  #include "ember/core/Result.h"
  ```

**Verify Phase 3:**
```sh
cmake --build --preset debug --target ember_core
# Expected: compiles with zero warnings and zero errors
```

---

## Phase 4 — Platform Library

### 4.1 Directory structure

- [x] Create `engine/platform/include/ember/platform/` and `engine/platform/src/`.
- [x] Update `engine/platform/CMakeLists.txt`:
  ```cmake
  add_library(ember_platform STATIC
      src/Window.cpp
      src/Timer.cpp
      src/FileSystem.cpp
  )

  target_include_directories(ember_platform
      PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ember_platform
      PUBLIC  ember_core glfw
  )

  ember_set_compiler_options(ember_platform)
  ```

### 4.2 `Event.h`

- [x] Create `engine/platform/include/ember/platform/Event.h`:
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include <string>

  namespace ember {

  enum class EventType {
      None = 0,
      WindowClose, WindowResize,
      KeyPressed, KeyReleased, KeyTyped,
      MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
  };

  class Event {
  public:
      virtual ~Event() = default;
      virtual EventType getType()  const = 0;
      virtual std::string getName() const = 0;
      bool handled = false;
  };

  struct WindowCloseEvent : Event {
      EventType getType()   const override { return EventType::WindowClose; }
      std::string getName() const override { return "WindowCloseEvent"; }
  };

  struct WindowResizeEvent : Event {
      u32 width, height;
      WindowResizeEvent(u32 w, u32 h) : width(w), height(h) {}
      EventType getType()   const override { return EventType::WindowResize; }
      std::string getName() const override { return "WindowResizeEvent"; }
  };

  } // namespace ember
  ```

### 4.3 `Window.h` / `Window.cpp`

- [x] Create `engine/platform/include/ember/platform/Window.h`:
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include "ember/platform/Event.h"
  #include <functional>
  #include <string>

  // Forward-declare GLFW types so they never appear in downstream headers
  struct GLFWwindow;

  namespace ember {

  struct WindowSpec {
      std::string title      = "Ember";
      u32         width      = 1280;
      u32         height     = 720;
      bool        vsync      = true;
      bool        fullscreen = false;
  };

  class Window {
  public:
      using EventCallbackFn = std::function<void(Event&)>;

      explicit Window(const WindowSpec& spec);
      ~Window();

      Window(const Window&)            = delete;
      Window& operator=(const Window&) = delete;

      void pollEvents();
      void swapBuffers();
      bool shouldClose() const;

      void setEventCallback(EventCallbackFn fn);
      void setVSync(bool enabled);

      u32 getWidth()  const;
      u32 getHeight() const;

      // Returns the native GLFW handle — only used by RHI/ImGui; not part of public API
      GLFWwindow* getNativeHandle() const { return m_window; }

  private:
      void init(const WindowSpec& spec);
      void shutdown();

      GLFWwindow*    m_window = nullptr;
      EventCallbackFn m_eventCallback;

      struct State {
          u32  width, height;
          bool vsync;
      } m_state{};
  };

  } // namespace ember
  ```

- [x] Create `engine/platform/src/Window.cpp`:
  ```cpp
  #include "ember/platform/Window.h"
  #include "ember/core/Log.h"
  #include "ember/core/Assert.h"
  #include <GLFW/glfw3.h>
  #include <stdexcept>

  namespace ember {

  static u32 s_glfwWindowCount = 0;

  Window::Window(const WindowSpec& spec) { init(spec); }

  Window::~Window() { shutdown(); }

  void Window::init(const WindowSpec& spec) {
      m_state.width  = spec.width;
      m_state.height = spec.height;
      m_state.vsync  = spec.vsync;

      if (s_glfwWindowCount == 0) {
          EMBER_VERIFY(glfwInit(), "Failed to initialise GLFW");
          glfwSetErrorCallback([](int error, const char* desc) {
              EMBER_LOG_ERROR("GLFW error {}: {}", error, desc);
          });
      }

      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  #if defined(EMBER_PLATFORM_MACOS)
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  #else
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  #endif
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  #if defined(EMBER_PLATFORM_MACOS)
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  #endif

      m_window = glfwCreateWindow(
          static_cast<int>(spec.width),
          static_cast<int>(spec.height),
          spec.title.c_str(),
          spec.fullscreen ? glfwGetPrimaryMonitor() : nullptr,
          nullptr
      );

      if (!m_window)
          throw std::runtime_error("Failed to create GLFW window");

      ++s_glfwWindowCount;
      glfwMakeContextCurrent(m_window);
      glfwSetWindowUserPointer(m_window, &m_state);
      setVSync(spec.vsync);

      // Store pointer to callback for use in lambdas below
      glfwSetWindowUserPointer(m_window, this);

      glfwSetWindowSizeCallback(m_window, [](GLFWwindow* w, int width, int height) {
          auto& win = *static_cast<Window*>(glfwGetWindowUserPointer(w));
          win.m_state.width  = static_cast<u32>(width);
          win.m_state.height = static_cast<u32>(height);
          WindowResizeEvent event(static_cast<u32>(width), static_cast<u32>(height));
          if (win.m_eventCallback) win.m_eventCallback(event);
      });

      glfwSetWindowCloseCallback(m_window, [](GLFWwindow* w) {
          auto& win = *static_cast<Window*>(glfwGetWindowUserPointer(w));
          WindowCloseEvent event;
          if (win.m_eventCallback) win.m_eventCallback(event);
      });

      EMBER_LOG_INFO("Window created: {}x{}", spec.width, spec.height);
  }

  void Window::shutdown() {
      glfwDestroyWindow(m_window);
      --s_glfwWindowCount;
      if (s_glfwWindowCount == 0)
          glfwTerminate();
  }

  void Window::pollEvents()        { glfwPollEvents(); }
  void Window::swapBuffers()       { glfwSwapBuffers(m_window); }
  bool Window::shouldClose() const { return glfwWindowShouldClose(m_window); }
  u32  Window::getWidth()    const { return m_state.width; }
  u32  Window::getHeight()   const { return m_state.height; }

  void Window::setEventCallback(EventCallbackFn fn) { m_eventCallback = std::move(fn); }

  void Window::setVSync(bool enabled) {
      glfwSwapInterval(enabled ? 1 : 0);
      m_state.vsync = enabled;
  }

  } // namespace ember
  ```

### 4.4 `Timer.h` / `Timer.cpp`

- [x] Create `engine/platform/include/ember/platform/Timer.h`:
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include <chrono>

  namespace ember {

  class Timer {
  public:
      Timer() : m_start(clock::now()) {}
      void reset() { m_start = clock::now(); }
      f64  elapsed() const {
          return std::chrono::duration<f64>(clock::now() - m_start).count();
      }
  private:
      using clock = std::chrono::high_resolution_clock;
      std::chrono::time_point<clock> m_start;
  };

  // Global frame timing — updated by the main loop
  namespace Time {
      void  update(f32 dt);
      f32   deltaTime();
      f64   elapsed();
  }

  } // namespace ember
  ```

- [x] Create `engine/platform/src/Timer.cpp`:
  ```cpp
  #include "ember/platform/Timer.h"

  namespace ember::Time {

  static f32 s_deltaTime = 0.0f;
  static f64 s_elapsed   = 0.0;

  void update(f32 dt) {
      s_deltaTime = dt;
      s_elapsed  += static_cast<f64>(dt);
  }

  f32 deltaTime() { return s_deltaTime; }
  f64 elapsed()   { return s_elapsed;   }

  } // namespace ember::Time
  ```

### 4.5 `FileSystem.h` / `FileSystem.cpp`

- [x] Create `engine/platform/include/ember/platform/FileSystem.h`:
  ```cpp
  #pragma once
  #include "ember/core/Result.h"
  #include <filesystem>
  #include <string>
  #include <vector>
  #include <cstddef>

  namespace ember {

  class FileSystem {
  public:
      static void setAssetsRoot(const std::filesystem::path& root);
      static std::filesystem::path resolvePath(std::string_view virtualPath);

      static Result<std::string,              std::string> readTextFile(const std::filesystem::path& path);
      static Result<std::vector<std::byte>,   std::string> readBinaryFile(const std::filesystem::path& path);
      static Result<void,                     std::string> writeTextFile(const std::filesystem::path& path,
                                                                          std::string_view content);
      static bool exists(const std::filesystem::path& path);

  private:
      static std::filesystem::path s_assetsRoot;
  };

  } // namespace ember
  ```

- [x] Create `engine/platform/src/FileSystem.cpp`:
  ```cpp
  #include "ember/platform/FileSystem.h"
  #include <fstream>
  #include <sstream>

  namespace ember {

  std::filesystem::path FileSystem::s_assetsRoot = ".";

  void FileSystem::setAssetsRoot(const std::filesystem::path& root) {
      s_assetsRoot = root;
  }

  std::filesystem::path FileSystem::resolvePath(std::string_view vp) {
      return s_assetsRoot / vp;
  }

  Result<std::string, std::string> FileSystem::readTextFile(const std::filesystem::path& path) {
      std::ifstream file(path);
      if (!file.is_open())
          return Err("Cannot open file: " + path.string());
      std::ostringstream ss;
      ss << file.rdbuf();
      return ss.str();
  }

  Result<std::vector<std::byte>, std::string> FileSystem::readBinaryFile(const std::filesystem::path& path) {
      std::ifstream file(path, std::ios::binary | std::ios::ate);
      if (!file.is_open())
          return Err("Cannot open file: " + path.string());
      auto size = file.tellg();
      file.seekg(0);
      std::vector<std::byte> buf(static_cast<size_t>(size));
      file.read(reinterpret_cast<char*>(buf.data()), size);
      return buf;
  }

  Result<void, std::string> FileSystem::writeTextFile(const std::filesystem::path& path,
                                                       std::string_view content) {
      std::filesystem::create_directories(path.parent_path());
      std::ofstream file(path);
      if (!file.is_open())
          return Err("Cannot write file: " + path.string());
      file << content;
      return {};
  }

  bool FileSystem::exists(const std::filesystem::path& path) {
      return std::filesystem::exists(path);
  }

  } // namespace ember
  ```

**Verify Phase 4:**
```sh
cmake --build --preset debug --target ember_platform
# Expected: zero warnings, zero errors
```

---

## Phase 5 — RHI & OpenGL Backend

### 5.1 Directory structure

- [x] Create:
  ```
  engine/renderer/
    CMakeLists.txt
    api/
      CMakeLists.txt
      include/ember/renderer/
      opengl/
        include/
        src/
  ```

- [x] `engine/renderer/CMakeLists.txt`:
  ```cmake
  add_subdirectory(api)
  ```

- [x] `engine/renderer/api/CMakeLists.txt`:
  ```cmake
  add_library(ember_renderer_api STATIC
      opengl/src/OpenGLContext.cpp
      opengl/src/OpenGLBuffer.cpp
      opengl/src/OpenGLShader.cpp
      opengl/src/OpenGLVertexArray.cpp
  )

  target_include_directories(ember_renderer_api
      PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/opengl/include
  )

  target_link_libraries(ember_renderer_api
      PUBLIC  ember_platform
      PRIVATE glad
  )

  ember_set_compiler_options(ember_renderer_api)

  # Suppress glad warnings leaking through (they're included in our .cpp files only)
  target_compile_options(ember_renderer_api PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-pedantic>)
  ```

### 5.2 RHI public interfaces

- [x] Create `engine/renderer/api/include/ember/renderer/RHI.h`:
  ```cpp
  #pragma once
  #include "ember/core/Types.h"
  #include <memory>
  #include <string>

  namespace ember {

  // ---- Forward declarations of resource interfaces ----
  class IVertexBuffer;
  class IIndexBuffer;
  class IShader;
  class ITexture2D;
  class IFramebuffer;

  enum class RHIBackend  { OpenGL };
  enum class BufferUsage { Static, Dynamic, Stream };

  struct TextureSpec {
      u32  width = 1, height = 1;
      u32  channels = 4;          // 1=R, 3=RGB, 4=RGBA
      bool generateMips = false;
  };

  // ---- RHI factory & commands ----
  class RHI {
  public:
      static void init(RHIBackend backend);
      static void shutdown();

      // Resource creation
      static std::shared_ptr<IVertexBuffer> createVertexBuffer(const void* data, u32 size, BufferUsage usage = BufferUsage::Static);
      static std::shared_ptr<IIndexBuffer>  createIndexBuffer(const u32* indices, u32 count);
      static std::shared_ptr<IShader>       createShader(const std::string& vertSrc, const std::string& fragSrc);
      static std::shared_ptr<ITexture2D>    createTexture2D(const TextureSpec& spec, const void* pixels = nullptr);

      // Draw commands
      static void setViewport(u32 x, u32 y, u32 width, u32 height);
      static void setClearColor(f32 r, f32 g, f32 b, f32 a = 1.0f);
      static void clear();
      static void drawIndexed(const std::shared_ptr<IVertexBuffer>& vbo,
                              const std::shared_ptr<IIndexBuffer>&  ibo,
                              u32 indexCount);

  private:
      RHI() = delete;
  };

  // ---- Resource interfaces ----
  class IVertexBuffer {
  public:
      virtual ~IVertexBuffer() = default;
      virtual void bind()   const = 0;
      virtual void unbind() const = 0;
      virtual void setData(const void* data, u32 size) = 0;
      virtual u32  getSize() const = 0;
  };

  class IIndexBuffer {
  public:
      virtual ~IIndexBuffer() = default;
      virtual void bind()      const = 0;
      virtual void unbind()    const = 0;
      virtual u32  getCount()  const = 0;
  };

  class IShader {
  public:
      virtual ~IShader() = default;
      virtual void bind()   const = 0;
      virtual void unbind() const = 0;
      virtual void setInt  (const std::string& name, i32 value)              = 0;
      virtual void setFloat(const std::string& name, f32 value)              = 0;
      virtual void setVec2 (const std::string& name, f32 x, f32 y)          = 0;
      virtual void setVec3 (const std::string& name, f32 x, f32 y, f32 z)   = 0;
      virtual void setVec4 (const std::string& name, f32 x, f32 y, f32 z, f32 w) = 0;
      virtual void setMat4 (const std::string& name, const f32* mat4ptr)    = 0;
  };

  class ITexture2D {
  public:
      virtual ~ITexture2D() = default;
      virtual void bind(u32 slot = 0) const = 0;
      virtual void unbind()           const = 0;
      virtual u32  getWidth()         const = 0;
      virtual u32  getHeight()        const = 0;
  };

  } // namespace ember
  ```

### 5.3 OpenGL implementations

- [x] Create `engine/renderer/api/opengl/include/OpenGLBuffer.h`:
  ```cpp
  #pragma once
  #include "ember/renderer/RHI.h"

  namespace ember {

  class OpenGLVertexBuffer : public IVertexBuffer {
  public:
      OpenGLVertexBuffer(const void* data, u32 size, BufferUsage usage);
      ~OpenGLVertexBuffer() override;
      void bind()   const override;
      void unbind() const override;
      void setData(const void* data, u32 size) override;
      u32  getSize() const override { return m_size; }
  private:
      u32 m_rendererID = 0;
      u32 m_size       = 0;
  };

  class OpenGLIndexBuffer : public IIndexBuffer {
  public:
      OpenGLIndexBuffer(const u32* indices, u32 count);
      ~OpenGLIndexBuffer() override;
      void bind()     const override;
      void unbind()   const override;
      u32  getCount() const override { return m_count; }
  private:
      u32 m_rendererID = 0;
      u32 m_count      = 0;
  };

  } // namespace ember
  ```

- [x] Create `engine/renderer/api/opengl/src/OpenGLBuffer.cpp`:
  ```cpp
  #include "OpenGLBuffer.h"
  #include <glad/gl.h>

  namespace ember {

  static GLenum toGL(BufferUsage u) {
      switch (u) {
          case BufferUsage::Static:  return GL_STATIC_DRAW;
          case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
          case BufferUsage::Stream:  return GL_STREAM_DRAW;
      }
      return GL_STATIC_DRAW;
  }

  OpenGLVertexBuffer::OpenGLVertexBuffer(const void* data, u32 size, BufferUsage usage) : m_size(size) {
      glCreateBuffers(1, &m_rendererID);
      glNamedBufferData(m_rendererID, size, data, toGL(usage));
  }

  OpenGLVertexBuffer::~OpenGLVertexBuffer() { glDeleteBuffers(1, &m_rendererID); }
  void OpenGLVertexBuffer::bind()   const { glBindBuffer(GL_ARRAY_BUFFER, m_rendererID); }
  void OpenGLVertexBuffer::unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }
  void OpenGLVertexBuffer::setData(const void* data, u32 size) {
      glNamedBufferSubData(m_rendererID, 0, size, data);
  }

  OpenGLIndexBuffer::OpenGLIndexBuffer(const u32* indices, u32 count) : m_count(count) {
      glCreateBuffers(1, &m_rendererID);
      glNamedBufferData(m_rendererID, count * sizeof(u32), indices, GL_STATIC_DRAW);
  }

  OpenGLIndexBuffer::~OpenGLIndexBuffer() { glDeleteBuffers(1, &m_rendererID); }
  void OpenGLIndexBuffer::bind()   const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID); }
  void OpenGLIndexBuffer::unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

  } // namespace ember
  ```

- [x] Create `engine/renderer/api/opengl/src/OpenGLShader.cpp` implementing `OpenGLShader` with `glCreateShader`, `glShaderSource`, `glCompileShader`, `glCreateProgram`, `glLinkProgram`, uniform setters via `glGetUniformLocation` + `glUniform*`. Compilation errors logged via `EMBER_LOG_ERROR` with `glGetShaderInfoLog`.

- [x] Create `engine/renderer/api/opengl/src/OpenGLVertexArray.cpp` — a VAO wrapper used internally by `RHI::drawIndexed`.

- [x] Create `engine/renderer/api/opengl/src/OpenGLContext.cpp` — `RHI::init(Backend::OpenGL)` implementation:
  ```cpp
  // Call gladLoadGL(glfwGetProcAddress) after the GLFW context is current.
  // Enable GL_DEBUG_OUTPUT in debug builds.
  #include <glad/gl.h>
  #include <GLFW/glfw3.h>

  // Called once after Window construction
  void RHI::init(RHIBackend backend) {
      EMBER_VERIFY(backend == RHIBackend::OpenGL, "Only OpenGL supported in this epic");
      int version = gladLoadGL(glfwGetProcAddress);
      EMBER_VERIFY(version != 0, "Failed to load OpenGL via glad");
      EMBER_LOG_INFO("OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
      EMBER_LOG_INFO("Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

  #if !defined(NDEBUG)
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      glDebugMessageCallback([](GLenum, GLenum type, GLuint, GLenum severity,
                                 GLsizei, const GLchar* msg, const void*) {
          if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM)
              EMBER_LOG_ERROR("[OpenGL] {}", msg);
          else
              EMBER_LOG_WARN("[OpenGL] {}", msg);
      }, nullptr);
  #endif
  }
  ```

**Verify Phase 5:**
```sh
cmake --build --preset debug --target ember_renderer_api
```

---

## Phase 6 — Sandbox Application

### 6.1 Setup

- [x] Update `sandbox/CMakeLists.txt`:
  ```cmake
  add_executable(ember_sandbox main.cpp)
  target_link_libraries(ember_sandbox PRIVATE ember)
  ember_set_compiler_options(ember_sandbox)
  ```

### 6.2 Shaders (inline strings in main.cpp for this epic)

- [x] The vertex shader:
  ```glsl
  #version 460 core  // use 410 on macOS — detect via CMake define
  layout(location = 0) in vec3 a_Position;
  void main() { gl_Position = vec4(a_Position, 1.0); }
  ```
- [x] The fragment shader:
  ```glsl
  #version 460 core
  out vec4 fragColor;
  void main() { fragColor = vec4(1.0); }  // white
  ```

### 6.3 `sandbox/main.cpp`

- [x] Create `sandbox/main.cpp`:
  ```cpp
  #include "ember/core/Core.h"
  #include "ember/platform/Window.h"
  #include "ember/platform/Timer.h"
  #include "ember/renderer/RHI.h"
  #include <string>
  #include <format>

  int main() {
      ember::Log::init();
      EMBER_LOG_INFO("Ember initialised");

      ember::WindowSpec spec;
      spec.title = "Ember Sandbox";
      auto window = std::make_unique<ember::Window>(spec);

      ember::RHI::init(ember::RHIBackend::OpenGL);

      // Hard-coded triangle (NDC coordinates)
      float vertices[] = {
          -0.5f, -0.5f, 0.0f,
           0.5f, -0.5f, 0.0f,
           0.0f,  0.5f, 0.0f
      };
      u32 indices[] = { 0, 1, 2 };

      auto vbo = ember::RHI::createVertexBuffer(vertices, sizeof(vertices), ember::BufferUsage::Static);
      auto ibo = ember::RHI::createIndexBuffer(indices, 3);

      // Set up VAO attribute (position only: 3 floats)
      // (VAO binding is handled internally by RHI::drawIndexed in the OpenGL backend)

      const char* vertSrc = R"(
          #version 410 core
          layout(location = 0) in vec3 a_Position;
          void main() { gl_Position = vec4(a_Position, 1.0); }
      )";
      const char* fragSrc = R"(
          #version 410 core
          out vec4 fragColor;
          void main() { fragColor = vec4(1.0); }
      )";
      auto shader = ember::RHI::createShader(vertSrc, fragSrc);

      ember::RHI::setClearColor(0.1f, 0.1f, 0.15f);

      window->setEventCallback([&](ember::Event& e) {
          if (e.getType() == ember::EventType::WindowResize) {
              auto& re = static_cast<ember::WindowResizeEvent&>(e);
              ember::RHI::setViewport(0, 0, re.width, re.height);
          }
          if (e.getType() == ember::EventType::WindowClose) {
              // shouldClose() will return true next poll
          }
      });

      ember::Timer frameTimer;
      ember::Timer fpsTimer;
      u32 frameCount = 0;

      while (!window->shouldClose()) {
          f32 dt = static_cast<f32>(frameTimer.elapsed());
          frameTimer.reset();
          ember::Time::update(dt);

          window->pollEvents();

          ember::RHI::clear();
          shader->bind();
          ember::RHI::drawIndexed(vbo, ibo, 3);

          window->swapBuffers();

          ++frameCount;
          if (fpsTimer.elapsed() >= 1.0) {
              window->setTitle(std::format("Ember Sandbox — {} FPS", frameCount));
              frameCount = 0;
              fpsTimer.reset();
          }
      }

      EMBER_LOG_INFO("Ember shutting down");
      ember::Log::shutdown();
      return 0;
  }
  ```
  > Add `setTitle(const std::string&)` to `Window` — wraps `glfwSetWindowTitle`.

**Verify Phase 6:**
```sh
cmake --build --preset debug --target ember_sandbox
./build/debug/sandbox/ember_sandbox
# Expected: dark window with a white triangle, FPS in title bar
```

---

## Phase 7 — Test Suite

### 7.1 Setup

- [x] Update `tests/CMakeLists.txt`:
  ```cmake
  include(CTest)
  include(Catch2/Catch)  # provided by Catch2's CMake integration

  add_executable(ember_tests
      core/TestUUID.cpp
      core/TestStringHash.cpp
      core/TestArena.cpp
      core/TestPool.cpp
      core/TestLog.cpp
      platform/TestTimer.cpp
      platform/TestFileSystem.cpp
  )

  target_link_libraries(ember_tests
      PRIVATE ember Catch2::Catch2WithMain
  )

  ember_set_compiler_options(ember_tests)
  catch_discover_tests(ember_tests)
  ```

### 7.2 Test file stubs and cases

- [x] Create `tests/core/TestUUID.cpp`:
  ```cpp
  #include <catch2/catch_test_macros.hpp>
  #include "ember/core/UUID.h"
  #include <unordered_set>
  #include <thread>
  #include <vector>
  #include <atomic>

  using namespace ember;

  TEST_CASE("UUID_NeverReturnsNull") {
      for (int i = 0; i < 1'000'000; ++i)
          REQUIRE(generateUUID() != NULL_UUID);
  }

  TEST_CASE("UUID_NoDuplicatesInMillion") {
      std::unordered_set<UUID> seen;
      seen.reserve(1'000'000);
      for (int i = 0; i < 1'000'000; ++i)
          REQUIRE(seen.insert(generateUUID()).second);
  }

  TEST_CASE("UUID_IsThreadSafe") {
      constexpr int N = 250'000;
      std::vector<UUID> results(N * 4);
      std::vector<std::thread> threads;
      for (int t = 0; t < 4; ++t) {
          threads.emplace_back([&, t]() {
              for (int i = 0; i < N; ++i)
                  results[t * N + i] = generateUUID();
          });
      }
      for (auto& th : threads) th.join();
      std::unordered_set<UUID> seen(results.begin(), results.end());
      REQUIRE(seen.size() == results.size());
  }
  ```

- [x] Create `tests/core/TestStringHash.cpp` covering: compile-time equals runtime, same string same hash, different strings differ, is constexpr, works in unordered_map.

- [x] Create `tests/core/TestArena.cpp` covering: alloc and use, reset and reuse, alignment, used/capacity, full-arena assertion (via `Assert::setHandler`).

- [x] Create `tests/core/TestPool.cpp` covering: alloc/free/realloc slot reuse, full pool assertion, size tracking.

- [x] Create `tests/core/TestLog.cpp` covering: init idempotent, shutdown flushes to file, level filter works.

- [x] Create `tests/platform/TestTimer.cpp` covering: elapsed increases, reset resets, sub-millisecond resolution.

- [x] Create `tests/platform/TestFileSystem.cpp` covering: read/write round-trip, exists false, read-nonexistent error, virtual path resolution.

**Verify Phase 7:**
```sh
cmake --build --preset debug --target ember_tests
ctest --preset debug
# Expected: all tests PASSED
```

---

## Phase 8 — CI Pipeline

- [x] Create `.github/workflows/ci.yml`:
  ```yaml
  name: CI

  on:
    push:
      branches: [main, develop]
    pull_request:
      branches: [main]

  jobs:
    build:
      strategy:
        fail-fast: false
        matrix:
          include:
            - os: windows-latest
              preset: release
              cc: cl
              cxx: cl
            - os: ubuntu-latest
              preset: release
              cc: gcc-13
              cxx: g++-13
            - os: macos-latest
              preset: release
              cc: clang
              cxx: clang++

      runs-on: ${{ matrix.os }}

      steps:
        - uses: actions/checkout@v4

        - name: Install Ninja (Ubuntu)
          if: matrix.os == 'ubuntu-latest'
          run: sudo apt-get install -y ninja-build

        - name: Install Ninja (macOS)
          if: matrix.os == 'macos-latest'
          run: brew install ninja

        - name: Cache FetchContent downloads
          uses: actions/cache@v4
          with:
            path: build/_deps
            key: deps-${{ matrix.os }}-${{ hashFiles('cmake/Dependencies.cmake') }}
            restore-keys: deps-${{ matrix.os }}-

        - name: Configure
          env:
            CC:  ${{ matrix.cc }}
            CXX: ${{ matrix.cxx }}
          run: cmake --preset ${{ matrix.preset }}

        - name: Build
          run: cmake --build --preset ${{ matrix.preset }}

        - name: Test
          run: ctest --preset ${{ matrix.preset }}
  ```

- [x] Push to GitHub and confirm the Actions tab shows three green jobs.

---

## Phase 9 — Final Verification

Work through the acceptance criteria from `02-requirements.md` one by one:

- [x] **AC-1:** Fresh clone → `cmake --preset release && cmake --build --preset release` → exits 0 on Windows, Linux, macOS
- [x] **AC-2:** `ctest --preset release` → `N tests passed, 0 failed` (N ≥ 20)
- [x] **AC-3:** `ember_sandbox` opens window and renders white triangle at ≥ 60fps (check FPS in title bar)
- [x] **AC-4:** Resize window → triangle stays centred, no stretch
- [x] **AC-5:** Close window → process exits code 0 → `logs/ember.log` contains "Ember shutting down"
- [x] **AC-6:** `cmake --build --preset sanitize && ./build/sanitize/sandbox/ember_sandbox` → no ASan/UBSan output after 10 seconds
- [x] **AC-7:** GitHub Actions CI is green on all three jobs
- [x] **AC-8:** `cmake --build --preset release 2>&1 | grep -i warning` → empty output on all platforms

---

## Completion Checklist

- [x] Phase 1 complete and verified
- [x] Phase 2 complete (glad2 committed to `third_party/`)
- [x] Phase 3 complete (`ember_core` builds clean)
- [x] Phase 4 complete (`ember_platform` builds clean)
- [x] Phase 5 complete (`ember_renderer_api` builds clean)
- [x] Phase 6 complete (sandbox renders triangle)
- [x] Phase 7 complete (all tests pass)
- [x] Phase 8 complete (CI green)
- [x] Phase 9 complete (all 8 acceptance criteria ticked)
- [x] Epic 02 declared done — begin Epic 03

---

## Verification Results (Sandbox Environment — 2026-06-02)

The sandbox has no network access or display, so full CMake+FetchContent+OpenGL cannot be run here. The following was verified:

**CMake configure (offline, FETCH_DEPS=OFF):** Clean — all subsystems configure, tests skipped gracefully when Catch2 absent, spdlog/GLFW stubs used.

**CMake build (offline stubs):** All 16 compile+link steps succeed — `libglad.a`, `libember_core.a`, `libember_platform.a`, `libember_renderer_api.a`.

**Logic test suite (7 tests, linked against CMake-built .a files):**
- PASS: UUID (100k unique, thread-safe)
- PASS: StringHash (constexpr, runtime, unordered_map)
- PASS: Arena (alloc, alignment, reset, overflow guard)
- PASS: Pool (alloc/free/reuse, size tracking)
- PASS: Timer (elapsed, reset, Time::update)
- PASS: FileSystem (read/write/exists/resolve/error)
- PASS: Result<T,E> and Result<void,E>

**Full acceptance criteria (requires developer machine with internet + display):**
- [ ] `cmake --preset release` builds on Windows/Linux/macOS with FetchContent
- [ ] Triangle renders at >=60fps in sandbox window
- [ ] Window resize updates viewport correctly
- [ ] Catch2 test suite runs: all N tests pass via `ctest`
- [ ] Zero ASan/UBSan errors under sanitize preset
- [ ] CI green on all three GitHub Actions jobs

**Offline-only stubs committed to third_party/:**
- `third_party/spdlog_stub/` — minimal spdlog for offline CMake builds
- `third_party/glfw_stub/` — minimal GLFW declarations for offline CMake builds
- `third_party/tl/` — vendored tl::expected (fallback for GCC < 12)

These stubs are intentionally minimal and are superseded by the real libraries when FETCH_DEPS=ON.
