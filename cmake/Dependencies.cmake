include(FetchContent)
option(EMBER_FETCH_DEPS "Fetch dependencies via FetchContent" ON)

if(NOT EMBER_FETCH_DEPS)
    return()
endif()

message(STATUS "Ember: Fetching dependencies...")

# ---- GLFW — windowing ----
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

# ---- GLM — math (header-only) ----
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)

# ---- EnTT — ECS (header-only) ----
FetchContent_Declare(EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.13.2
    GIT_SHALLOW    TRUE
)

# ---- spdlog — logging ----
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)
set(SPDLOG_BUILD_SHARED  OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL       OFF CACHE BOOL "" FORCE)

# ---- yaml-cpp — serialization ----
FetchContent_Declare(yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        0.8.0
    GIT_SHALLOW    TRUE
)
set(YAML_CPP_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS   OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL       OFF CACHE BOOL "" FORCE)

# ---- Catch2 — testing ----
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
    GIT_SHALLOW    TRUE
)

# ---- Dear ImGui (docking branch) — editor UI ----
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.9-docking
    GIT_SHALLOW    TRUE
)

# ---- stb — image / font loading ----
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        5736b15f7ea0ffb08dd38af21067c314d6a3aae9
    GIT_SHALLOW    TRUE
)

# Fetch everything (CMake parallelises the downloads)
FetchContent_MakeAvailable(glfw glm EnTT spdlog yaml-cpp Catch2)

# stb and imgui are consumed via SYSTEM includes — just populate
FetchContent_Populate(stb   QUIET)
FetchContent_Populate(imgui QUIET)

# Expose stb include path as a variable for targets that need it
set(STB_INCLUDE_DIR "${stb_SOURCE_DIR}" CACHE PATH "stb include directory" FORCE)
set(IMGUI_SOURCE_DIR "${imgui_SOURCE_DIR}" CACHE PATH "ImGui source directory" FORCE)

message(STATUS "Ember: Dependencies ready")
