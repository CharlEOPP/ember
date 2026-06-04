add_library(ember_physics2d STATIC
    src/Physics2DSystem.cpp
    src/Physics2DComponents.cpp
)

target_include_directories(ember_physics2d
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ecs brings World/ISystem/Transform/Reflect (+ core, glm). serialization +
# yaml-cpp are PRIVATE: only Physics2DComponents.cpp uses them to register
# components, so yaml never leaks into the physics public headers.
target_link_libraries(ember_physics2d
    PUBLIC  ember_core ember_ecs glm::glm
    PRIVATE ember_serialization yaml-cpp::yaml-cpp ember_scripting
)

target_compile_features(ember_physics2d PUBLIC cxx_std_20)
ember_set_compiler_options(ember_physics2d)
