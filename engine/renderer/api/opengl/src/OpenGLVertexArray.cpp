#include "OpenGLVertexArray.h"
#include <glad/gl.h>

namespace ember {

OpenGLVertexArray::OpenGLVertexArray() {
    glGenVertexArrays(1, &m_rendererID);
}

OpenGLVertexArray::~OpenGLVertexArray() {
    glDeleteVertexArrays(1, &m_rendererID);
}

void OpenGLVertexArray::bind()   const { glBindVertexArray(m_rendererID); }
void OpenGLVertexArray::unbind() const { glBindVertexArray(0); }

void OpenGLVertexArray::attachVertexBuffer(const std::shared_ptr<IVertexBuffer>& vbo,
                                            u32 attribIndex, i32 componentCount,
                                            u32 stride, usize offset)
{
    bind();
    vbo->bind();
    glEnableVertexAttribArray(attribIndex);
    glVertexAttribPointer(attribIndex, componentCount, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(stride),
                          reinterpret_cast<const void*>(offset));
}

void OpenGLVertexArray::attachIndexBuffer(const std::shared_ptr<IIndexBuffer>& ibo) {
    bind();
    ibo->bind(); // binding IBO while VAO is bound stores it in the VAO
}

} // namespace ember
