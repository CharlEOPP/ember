#include "ember/renderer/RHI.h"
#include "OpenGLBuffer.h"
#include "OpenGLShader.h"
#include "OpenGLVertexArray.h"
#include "ember/core/Log.h"
#include "ember/core/Assert.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace ember {

// ---- RHI::init ----

void RHI::init(RHIBackend backend) {
    EMBER_VERIFY(backend == RHIBackend::OpenGL, "Only OpenGL backend is supported in Epic 02");

    const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress));
    EMBER_VERIFY(version != 0, "Failed to load OpenGL via glad");

    EMBER_LOG_INFO("OpenGL {}.{} loaded",
        GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    EMBER_LOG_INFO("  Renderer: {}",
        reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    EMBER_LOG_INFO("  Vendor:   {}",
        reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

    // Debug output — optional on 4.1 (skip if not available)
#if !defined(NDEBUG)
    if (glad_glDebugMessageCallback) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback([](GLenum /*source*/, GLenum /*type*/, GLuint /*id*/,
                                   GLenum severity, GLsizei /*length*/,
                                   const GLchar* message, const void* /*userParam*/)
        {
            if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM)
                EMBER_LOG_ERROR("[OpenGL] {}", message);
            else if (severity == GL_DEBUG_SEVERITY_LOW)
                EMBER_LOG_WARN("[OpenGL] {}", message);
        }, nullptr);
        EMBER_LOG_DEBUG("OpenGL debug output enabled");
    }
#endif
}

void RHI::shutdown() {
    // Nothing to do for OpenGL — context is owned by the Window
}

// ---- Resource creation ----

std::shared_ptr<IVertexBuffer> RHI::createVertexBuffer(const void* data, u32 size, BufferUsage usage) {
    return std::make_shared<OpenGLVertexBuffer>(data, size, usage);
}

std::shared_ptr<IIndexBuffer> RHI::createIndexBuffer(const u32* indices, u32 count) {
    return std::make_shared<OpenGLIndexBuffer>(indices, count);
}

std::shared_ptr<IShader> RHI::createShader(const std::string& vertSrc, const std::string& fragSrc) {
    auto shader = std::make_shared<OpenGLShader>(vertSrc, fragSrc);
    if (!shader->isValid()) {
        EMBER_LOG_ERROR("RHI::createShader failed — returning nullptr");
        return nullptr;
    }
    return shader;
}

std::shared_ptr<ITexture2D> RHI::createTexture2D([[maybe_unused]] const TextureSpec& spec,
                                                   [[maybe_unused]] const void* pixels) {
    // Texture2D implementation deferred to Epic 04 (Renderer 2D)
    EMBER_ASSERT(false, "RHI::createTexture2D not yet implemented");
    return nullptr;
}

// ---- Draw commands ----

void RHI::setViewport(u32 x, u32 y, u32 width, u32 height) {
    glViewport(static_cast<GLint>(x),  static_cast<GLint>(y),
               static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

void RHI::setClearColor(f32 r, f32 g, f32 b, f32 a) {
    glClearColor(r, g, b, a);
}

void RHI::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RHI::drawIndexed(const std::shared_ptr<IVertexBuffer>& vbo,
                      const std::shared_ptr<IIndexBuffer>&  ibo,
                      u32 indexCount)
{
    // For simplicity in Epic 02 we create a transient VAO each call.
    // Epic 04 (Renderer2D) will cache VAOs per vertex layout.
    OpenGLVertexArray vao;
    // Position only: 3 floats, location 0
    vao.attachVertexBuffer(vbo, 0, 3, sizeof(f32) * 3, 0);
    vao.attachIndexBuffer(ibo);
    vao.bind();

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
                   GL_UNSIGNED_INT, nullptr);

    vao.unbind();
}

} // namespace ember
