add_library(ember_renderer_2d STATIC
    src/Renderer2D.cpp
    src/DebugDraw.cpp
    loaders/Texture2DLoader.cpp
    loaders/FontAsset.cpp
    loaders/stb_impl.cpp
    systems/SpriteRenderSystem.cpp
    ../camera/src/CameraSystem.cpp
    ../camera/src/EditorCamera.cpp
)

target_include_directories(ember_renderer_2d
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../camera/include
    PRIVATE ${STB_INCLUDE_DIR}            # stb_image.h (PRIVATE: never leaks to consumers)
)

target_link_libraries(ember_renderer_2d
    PUBLIC ember_core ember_ecs ember_scene ember_renderer_api glm::glm
)

target_compile_features(ember_renderer_2d PUBLIC cxx_std_20)
ember_set_compiler_options(ember_renderer_2d)
