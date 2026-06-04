add_library(ember_serialization STATIC
    src/YAMLSerializer.cpp
    src/YAMLDeserializer.cpp
    src/SceneSerialization.cpp
)

target_include_directories(ember_serialization
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# yaml-cpp is PRIVATE so it never leaks into consumers' include paths (NFR-04).
target_link_libraries(ember_serialization
    PUBLIC  ember_core ember_ecs ember_scene
    PRIVATE yaml-cpp::yaml-cpp
)

target_compile_features(ember_serialization PUBLIC cxx_std_20)
ember_set_compiler_options(ember_serialization)
