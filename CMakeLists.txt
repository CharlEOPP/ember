cmake_minimum_required(VERSION 3.25)
project(Ember VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- CMake modules ----
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CompilerOptions)
include(Platform)
include(Dependencies)

# ---- glad2 (pre-generated, vendored) ----
add_subdirectory(third_party/glad)

# ---- Engine subsystems ----
add_subdirectory(engine/core)
add_subdirectory(engine/platform)
add_subdirectory(engine/events)
add_subdirectory(engine/ecs)
add_subdirectory(engine/scene)
add_subdirectory(engine/serialization)
add_subdirectory(engine/renderer)

# ---- Aggregate interface target ----
add_library(ember INTERFACE)
target_link_libraries(ember INTERFACE
    ember_core
    ember_platform
    ember_events
    ember_ecs
    ember_scene
    ember_serialization
    ember_renderer_api
    ember_renderer_2d
)

# ---- Applications ----
add_subdirectory(sandbox)

# ---- Tests ----
enable_testing()
add_subdirectory(tests)
