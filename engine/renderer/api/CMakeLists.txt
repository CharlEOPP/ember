add_library(ember_renderer_api STATIC
    src/RHILayout.cpp
    src/ShaderLibrary.cpp
    opengl/src/OpenGLContext.cpp
    opengl/src/OpenGLBuffer.cpp
    opengl/src/OpenGLShader.cpp
    opengl/src/OpenGLVertexArray.cpp
    opengl/src/OpenGLTexture2D.cpp
    opengl/src/OpenGLFramebuffer.cpp
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
