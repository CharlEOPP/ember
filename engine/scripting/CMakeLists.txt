add_library(ember_scripting STATIC
    src/ScriptComponent.cpp
    src/ScriptRegistry.cpp
    src/ScriptSystem.cpp
)

target_include_directories(ember_scripting
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# core/ecs/scene/assets are PUBLIC because ScriptComponent.h exposes their types
# (World, Scene, Transform, AssetHandle). serialization + yaml-cpp are only used
# by code that includes RegisterScript.h (game registration TUs); kept PRIVATE so
# yaml never leaks through the scripting public headers.
target_link_libraries(ember_scripting
    PUBLIC  ember_core ember_ecs ember_scene ember_assets glm::glm
    PRIVATE ember_serialization yaml-cpp::yaml-cpp
)

target_compile_features(ember_scripting PUBLIC cxx_std_20)
ember_set_compiler_options(ember_scripting)
