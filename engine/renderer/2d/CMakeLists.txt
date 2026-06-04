add_library(ember_renderer_2d STATIC
    src/Renderer2D.cpp
    src/DebugDraw.cpp
    systems/SpriteRenderSystem.cpp
    systems/RenderComponentSerialization.cpp
    ../camera/src/CameraSystem.cpp
    ../camera/src/EditorCamera.cpp
)

target_include_directories(ember_renderer_2d
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../camera/include
)

# FontAsset/Texture2D loaders + stb moved to ember_assets in Epic 05; link it
# for the FontAsset type used by Renderer2D::drawText. ember_serialization +
# yaml-cpp are PRIVATE: only RenderComponentSerialization.cpp uses them to
# register SpriteRenderer/Camera2D with the scene serializer (MIG-04), so yaml
# never leaks into renderer headers.
target_link_libraries(ember_renderer_2d
    PUBLIC  ember_core ember_ecs ember_scene ember_renderer_api ember_assets glm::glm
    PRIVATE ember_serialization yaml-cpp::yaml-cpp
)

target_compile_features(ember_renderer_2d PUBLIC cxx_std_20)
ember_set_compiler_options(ember_renderer_2d)
