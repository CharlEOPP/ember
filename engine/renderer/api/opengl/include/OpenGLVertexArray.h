#pragma once
#include "ember/core/Types.h"
#include "ember/renderer/RHI.h"
#include <memory>

namespace ember {

/**
 * Wraps a GL VAO.  Used internally by RHI::drawIndexed to bind the
 * vertex layout (position-only: 3 floats at location 0) before drawing.
 */
class OpenGLVertexArray {
public:
    OpenGLVertexArray();
    ~OpenGLVertexArray();

    OpenGLVertexArray(const OpenGLVertexArray&)            = delete;
    OpenGLVertexArray& operator=(const OpenGLVertexArray&) = delete;

    void bind()   const;
    void unbind() const;

    /**
     * Attach a vertex buffer and declare the attribute layout.
     * For this epic: position only — 3 floats at attribute index 0.
     */
    void attachVertexBuffer(const std::shared_ptr<IVertexBuffer>& vbo,
                            u32 attribIndex, i32 componentCount,
                            u32 stride, usize offset);

    void attachIndexBuffer(const std::shared_ptr<IIndexBuffer>& ibo);

private:
    u32 m_rendererID = 0;
};

} // namespace ember
