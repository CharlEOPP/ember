#pragma once
#include "ember/renderer/RHI.h"

namespace ember {

class OpenGLVertexBuffer : public IVertexBuffer {
public:
    OpenGLVertexBuffer(const void* data, u32 size, BufferUsage usage);
    ~OpenGLVertexBuffer() override;

    void bind()   const override;
    void unbind() const override;
    void setData(const void* data, u32 size) override;
    u32  getSize() const override { return m_size; }

private:
    u32 m_rendererID = 0;
    u32 m_size       = 0;
};

class OpenGLIndexBuffer : public IIndexBuffer {
public:
    OpenGLIndexBuffer(const u32* indices, u32 count);
    ~OpenGLIndexBuffer() override;

    void bind()     const override;
    void unbind()   const override;
    u32  getCount() const override { return m_count; }

private:
    u32 m_rendererID = 0;
    u32 m_count      = 0;
};

} // namespace ember
