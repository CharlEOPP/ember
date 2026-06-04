#include "OpenGLFramebuffer.h"
#include "ember/core/Log.h"
#include <glad/gl.h>

namespace ember {

OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpec& spec) : m_spec(spec) {
    invalidate();
}

OpenGLFramebuffer::~OpenGLFramebuffer() { release(); }

void OpenGLFramebuffer::release() {
    if (m_rendererID)      glDeleteFramebuffers(1, &m_rendererID);
    if (m_colorAttachment) glDeleteTextures(1, &m_colorAttachment);
    if (m_depthAttachment) glDeleteRenderbuffers(1, &m_depthAttachment);
    m_rendererID = m_colorAttachment = m_depthAttachment = 0;
}

void OpenGLFramebuffer::invalidate() {
    release();

    glGenFramebuffers(1, &m_rendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);

    // Color attachment (sampleable texture).
    glGenTextures(1, &m_colorAttachment);
    glBindTexture(GL_TEXTURE_2D, m_colorAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 static_cast<GLsizei>(m_spec.width), static_cast<GLsizei>(m_spec.height), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorAttachment, 0);

    // Optional depth/stencil renderbuffer.
    if (m_spec.depth) {
        glGenRenderbuffers(1, &m_depthAttachment);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthAttachment);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                              static_cast<GLsizei>(m_spec.width), static_cast<GLsizei>(m_spec.height));
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, m_depthAttachment);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        EMBER_LOG_ERROR("Framebuffer is incomplete ({}x{})", m_spec.width, m_spec.height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLFramebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
    glViewport(0, 0, static_cast<GLsizei>(m_spec.width), static_cast<GLsizei>(m_spec.height));
}

void OpenGLFramebuffer::unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void OpenGLFramebuffer::resize(u32 w, u32 h) {
    if (w == 0 || h == 0) return;
    m_spec.width = w;
    m_spec.height = h;
    invalidate();
}

} // namespace ember
