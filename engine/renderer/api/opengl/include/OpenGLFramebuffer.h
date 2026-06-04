#pragma once
#include "ember/renderer/RHI.h"

namespace ember {

class OpenGLFramebuffer : public IFramebuffer {
public:
    explicit OpenGLFramebuffer(const FramebufferSpec& spec);
    ~OpenGLFramebuffer() override;

    void bind()   const override;
    void unbind() const override;
    void resize(u32 w, u32 h) override;
    u32  getColorAttachment() const override { return m_colorAttachment; }

private:
    void invalidate();
    void release();

    u32             m_rendererID      = 0;
    u32             m_colorAttachment = 0;
    u32             m_depthAttachment = 0;
    FramebufferSpec m_spec;
};

} // namespace ember
