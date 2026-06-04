#include "ember/renderer/RHI.h"
#include "OpenGLBuffer.h"
#include "OpenGLShader.h"
#include "OpenGLVertexArray.h"
#include "OpenGLTexture2D.h"
#include "OpenGLFramebuffer.h"
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

std::shared_ptr<IVertexBuffer> RHI::createVertexBuffer(u32 sizeBytes) {
    return std::make_shared<OpenGLVertexBuffer>(sizeBytes);
}

std::shared_ptr<IVertexArray> RHI::createVertexArray() {
    return std::make_shared<OpenGLVertexArray>();
}

std::shared_ptr<ITexture2D> RHI::createTexture2D(const TextureSpec& spec, const void* pixels) {
    return std::make_shared<OpenGLTexture2D>(spec, pixels);
}

std::shared_ptr<IFramebuffer> RHI::createFramebuffer(const FramebufferSpec& spec) {
    return std::make_shared<OpenGLFramebuffer>(spec);
}

std::shared_ptr<ITexture2D> RHI::whiteTexture() {
    static std::shared_ptr<ITexture2D> s_white;
    if (!s_white) {
        TextureSpec spec;
        spec.width = 1; spec.height = 1; spec.format = TextureFormat::RGBA8;
        const u32 pixel = 0xFFFFFFFFu;   // opaque white
        s_white = std::make_shared<OpenGLTexture2D>(spec, &pixel);
    }
    return s_white;
}

// ---- Draw commands ----

void RHI::setViewport(u32 x, u32 y, u32 width, u32 height) {
    glViewport(static_cast<GLint>(x),  static_cast<GLint>(y),
               static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

void RHI::setClearColor(f32 r, f32 g, f32 b, f32 a) {
    glClearColor(r, g, b, a);
}

void RHI::setBlend(bool enabled) {
    if (enabled) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
}

void RHI::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RHI::drawIndexed(const std::shared_ptr<IVertexArray>& vao, u32 indexCount) {
    vao->bind();
    const u32 count = indexCount ? indexCount : vao->indexCount();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(count), GL_UNSIGNED_INT, nullptr);
    vao->unbind();
}

void RHI::drawLines(const std::shared_ptr<IVertexArray>& vao, u32 vertexCount) {
    vao->bind();
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertexCount));
    vao->unbind();
}

void RHI::drawIndexed(const std::shared_ptr<IVertexBuffer>& vbo,
                      const std::shared_ptr<IIndexBuffer>&  ibo,
                      u32 indexCount)
{
    // Legacy path (Epic 02 triangle): ensure a default position layout, then draw
    // through the layout-aware VAO.
    if (vbo->getLayout().empty())
        const_cast<IVertexBuffer&>(*vbo).setLayout({ { ShaderDataType::Float3, "a_Position" } });

    auto vao = std::make_shared<OpenGLVertexArray>();
    vao->addVertexBuffer(vbo);
    vao->setIndexBuffer(ibo);
    vao->bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
    vao->unbind();
}

} // namespace ember
