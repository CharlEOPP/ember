add_library(ember_ecs STATIC
    src/World.cpp
    src/ComponentRegistry.cpp
    src/Components.cpp
)

target_include_directories(ember_ecs
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ember_ecs
    PUBLIC ember_core EnTT::EnTT glm::glm
)

# 64-bit entity ids — PUBLIC so every consumer compiles EnTT with the same id type.
target_compile_definitions(ember_ecs PUBLIC ENTT_ID_TYPE=std::uint64_t)

target_compile_features(ember_ecs PUBLIC cxx_std_20)
ember_set_compiler_options(ember_ecs)
