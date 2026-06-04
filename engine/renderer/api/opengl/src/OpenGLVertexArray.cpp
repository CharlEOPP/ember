#include "OpenGLVertexArray.h"
#include "ember/core/Assert.h"
#include <glad/gl.h>
namespace ember {
static GLenum baseGLType(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float: case ShaderDataType::Float2:
        case ShaderDataType::Float3: case ShaderDataType::Float4:
        case ShaderDataType::Mat3: case ShaderDataType::Mat4: return GL_FLOAT;
        case ShaderDataType::Int: case ShaderDataType::Int2:
        case ShaderDataType::Int3: case ShaderDataType::Int4: return GL_INT;
        case ShaderDataType::Bool: return GL_BOOL;
        case ShaderDataType::None: break;
    }
    EMBER_ASSERT(false, "Unknown ShaderDataType");
    return 0;
}
static bool isIntegerType(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Int: case ShaderDataType::Int2:
        case ShaderDataType::Int3: case ShaderDataType::Int4:
        case ShaderDataType::Bool: return true;
        default: return false;
    }
}
OpenGLVertexArray::OpenGLVertexArray() { glGenVertexArrays(1, &m_rendererID); }
OpenGLVertexArray::~OpenGLVertexArray() { glDeleteVertexArrays(1, &m_rendererID); }
void OpenGLVertexArray::bind() const { glBindVertexArray(m_rendererID); }
void OpenGLVertexArray::unbind() const { glBindVertexArray(0); }
void OpenGLVertexArray::addVertexBuffer(const std::shared_ptr<IVertexBuffer>& vbo) {
    EMBER_ASSERT(!vbo->getLayout().empty(), "Vertex buffer has no layout");
    glBindVertexArray(m_rendererID);
    vbo->bind();
    const BufferLayout& layout = vbo->getLayout();
    for (const BufferElement& e : layout.elements()) {
        const GLint count = static_cast<GLint>(shaderDataTypeComponentCount(e.type));
        const GLenum type = baseGLType(e.type);
        const GLsizei stride = static_cast<GLsizei>(layout.stride());
        const void* ptr = reinterpret_cast<const void*>(static_cast<usize>(e.offset));
        glEnableVertexAttribArray(m_attribIndex);
        if (isIntegerType(e.type))
            glVertexAttribIPointer(m_attribIndex, count, type, stride, ptr);
        else
            glVertexAttribPointer(m_attribIndex, count, type, e.normalized ? GL_TRUE : GL_FALSE, stride, ptr);
        ++m_attribIndex;
    }
    m_vertexBuffers.push_back(vbo);
    glBindVertexArray(0);
}
void OpenGLVertexArray::setIndexBuffer(const std::shared_ptr<IIndexBuffer>& ibo) {
    glBindVertexArray(m_rendererID);
    ibo->bind();
    m_indexBuffer = ibo;
    glBindVertexArray(0);
}
u32 OpenGLVertexArray::indexCount() const { return m_indexBuffer ? m_indexBuffer->getCount() : 0; }
} // namespace ember
