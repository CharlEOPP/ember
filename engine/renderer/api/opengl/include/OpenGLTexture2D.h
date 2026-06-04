#pragma once
#include "ember/renderer/RHI.h"

namespace ember {

class OpenGLTexture2D : public ITexture2D {
public:
    OpenGLTexture2D(const TextureSpec& spec, const void* pixels);
    ~OpenGLTexture2D() override;

    void bind(u32 slot = 0) const override;
    void unbind()           const override;
    u32  getWidth()         const override { return m_width; }
    u32  getHeight()        const override { return m_height; }
    u32  rendererID()       const override { return m_rendererID; }

private:
    u32 m_rendererID = 0;
    u32 m_width  = 0;
    u32 m_height = 0;
};

} // namespace ember
