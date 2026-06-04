#pragma once
#include "ember/renderer/RHI.h"
#include <memory>
#include <vector>
namespace ember {
class OpenGLVertexArray : public IVertexArray {
public:
    OpenGLVertexArray();
    ~OpenGLVertexArray() override;
    OpenGLVertexArray(const OpenGLVertexArray&)=delete;
    OpenGLVertexArray& operator=(const OpenGLVertexArray&)=delete;
    void bind() const override; void unbind() const override;
    void addVertexBuffer(const std::shared_ptr<IVertexBuffer>& vbo) override;
    void setIndexBuffer(const std::shared_ptr<IIndexBuffer>& ibo) override;
    u32 indexCount() const override;
private:
    u32 m_rendererID=0; u32 m_attribIndex=0;
    std::vector<std::shared_ptr<IVertexBuffer>> m_vertexBuffers;
    std::shared_ptr<IIndexBuffer> m_indexBuffer;
};
} // namespace ember
