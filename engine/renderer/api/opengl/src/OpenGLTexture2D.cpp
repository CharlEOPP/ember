#include "OpenGLTexture2D.h"
#include <glad/gl.h>

namespace ember {

namespace {
struct GLFormat { GLint internalFormat; GLenum dataFormat; };

GLFormat toGLFormat(TextureFormat f) {
    switch (f) {
        case TextureFormat::RGBA8: return { GL_RGBA8, GL_RGBA };
        case TextureFormat::RGB8:  return { GL_RGB8,  GL_RGB  };
        case TextureFormat::R8:    return { GL_R8,    GL_RED  };
    }
    return { GL_RGBA8, GL_RGBA };
}

GLint toGLFilter(TextureFilter f, bool mips) {
    if (f == TextureFilter::Nearest)
        return mips ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
    return mips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
}

GLint toGLWrap(TextureWrap w) {
    return (w == TextureWrap::Repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
}
} // namespace

OpenGLTexture2D::OpenGLTexture2D(const TextureSpec& spec, const void* pixels)
    : m_width(spec.width), m_height(spec.height)
{
    const GLFormat fmt = toGLFormat(spec.format);

    glGenTextures(1, &m_rendererID);
    glBindTexture(GL_TEXTURE_2D, m_rendererID);

    // Tightly-packed rows for non-4-byte-aligned formats (e.g. R8 font atlases).
    glPixelStorei(GL_UNPACK_ALIGNMENT, (spec.format == TextureFormat::RGBA8) ? 4 : 1);

    glTexImage2D(GL_TEXTURE_2D, 0, fmt.internalFormat,
                 static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height), 0,
                 fmt.dataFormat, GL_UNSIGNED_BYTE, pixels);

    if (spec.generateMips && pixels)
        glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, toGLFilter(spec.filter, spec.generateMips));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, toGLFilter(spec.filter, false));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, toGLWrap(spec.wrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, toGLWrap(spec.wrap));

    glBindTexture(GL_TEXTURE_2D, 0);
}

OpenGLTexture2D::~OpenGLTexture2D() { glDeleteTextures(1, &m_rendererID); }

void OpenGLTexture2D::bind(u32 slot) const {
    // glActiveTexture + glBindTexture (4.1-compatible; glBindTextureUnit is 4.5).
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_rendererID);
}

void OpenGLTexture2D::unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

} // namespace ember
