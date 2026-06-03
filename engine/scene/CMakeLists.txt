add_library(ember_scene STATIC
    src/Scene.cpp
    src/SceneManager.cpp
    systems/TransformSystem.cpp
)

target_include_directories(ember_scene
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/systems
)

target_link_libraries(ember_scene
    PUBLIC ember_core ember_ecs ember_events glm::glm
)

target_compile_features(ember_scene PUBLIC cxx_std_20)
ember_set_compiler_options(ember_scene)
