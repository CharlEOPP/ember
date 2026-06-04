#include "OpenGLBuffer.h"
#include <glad/gl.h>
namespace ember {
static GLenum toGLUsage(BufferUsage u) {
    switch (u) {
        case BufferUsage::Static:  return GL_STATIC_DRAW;
        case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
        case BufferUsage::Stream:  return GL_STREAM_DRAW;
    }
    return GL_STATIC_DRAW;
}
OpenGLVertexBuffer::OpenGLVertexBuffer(const void* data, u32 size, BufferUsage usage) : m_size(size) {
    glGenBuffers(1, &m_rendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
    glBufferData(GL_ARRAY_BUFFER, size, data, toGLUsage(usage));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
OpenGLVertexBuffer::OpenGLVertexBuffer(u32 sizeBytes) : m_size(sizeBytes) {
    glGenBuffers(1, &m_rendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
    glBufferData(GL_ARRAY_BUFFER, sizeBytes, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
OpenGLVertexBuffer::~OpenGLVertexBuffer() { glDeleteBuffers(1, &m_rendererID); }
void OpenGLVertexBuffer::bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_rendererID); }
void OpenGLVertexBuffer::unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }
void OpenGLVertexBuffer::setData(const void* data, u32 size) {
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}
OpenGLIndexBuffer::OpenGLIndexBuffer(const u32* indices, u32 count) : m_count(count) {
    glGenBuffers(1, &m_rendererID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(count*sizeof(u32)), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
OpenGLIndexBuffer::~OpenGLIndexBuffer() { glDeleteBuffers(1, &m_rendererID); }
void OpenGLIndexBuffer::bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID); }
void OpenGLIndexBuffer::unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
} // namespace ember
