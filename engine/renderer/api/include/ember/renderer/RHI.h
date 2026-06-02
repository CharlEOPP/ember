#pragma once
#include "ember/core/Types.h"
#include <memory>
#include <string>

// No OpenGL or GLFW headers here — ever.

namespace ember {

// ---- Forward declarations ----
class IVertexBuffer;
class IIndexBuffer;
class IShader;
class ITexture2D;
class IFramebuffer;

enum class RHIBackend  { OpenGL };
enum class BufferUsage { Static, Dynamic, Stream };

struct TextureSpec {
    u32  width    = 1;
    u32  height   = 1;
    u32  channels = 4;          // 1=R  3=RGB  4=RGBA
    bool generateMips = false;
};

// ---- RHI factory & commands ----
class RHI {
public:
    static void init(RHIBackend backend);
    static void shutdown();

    // Resource creation
    static std::shared_ptr<IVertexBuffer> createVertexBuffer(const void* data, u32 size,
                                                              BufferUsage usage = BufferUsage::Static);
    static std::shared_ptr<IIndexBuffer>  createIndexBuffer(const u32* indices, u32 count);
    static std::shared_ptr<IShader>       createShader(const std::string& vertSrc,
                                                        const std::string& fragSrc);
    static std::shared_ptr<ITexture2D>    createTexture2D(const TextureSpec& spec,
                                                           const void* pixels = nullptr);

    // Draw commands
    static void setViewport(u32 x, u32 y, u32 width, u32 height);
    static void setClearColor(f32 r, f32 g, f32 b, f32 a = 1.0f);
    static void clear();
    static void drawIndexed(const std::shared_ptr<IVertexBuffer>& vbo,
                            const std::shared_ptr<IIndexBuffer>&  ibo,
                            u32 indexCount);

private:
    RHI() = delete;
};

// ---- Resource interfaces ----

class IVertexBuffer {
public:
    virtual ~IVertexBuffer() = default;
    virtual void bind()   const = 0;
    virtual void unbind() const = 0;
    virtual void setData(const void* data, u32 size) = 0;
    virtual u32  getSize() const = 0;
};

class IIndexBuffer {
public:
    virtual ~IIndexBuffer() = default;
    virtual void bind()     const = 0;
    virtual void unbind()   const = 0;
    virtual u32  getCount() const = 0;
};

class IShader {
public:
    virtual ~IShader() = default;
    virtual void bind()   const = 0;
    virtual void unbind() const = 0;
    virtual void setInt  (const std::string& name, i32 value)                    = 0;
    virtual void setFloat(const std::string& name, f32 value)                    = 0;
    virtual void setVec2 (const std::string& name, f32 x, f32 y)                = 0;
    virtual void setVec3 (const std::string& name, f32 x, f32 y, f32 z)         = 0;
    virtual void setVec4 (const std::string& name, f32 x, f32 y, f32 z, f32 w)  = 0;
    virtual void setMat4 (const std::string& name, const f32* mat4ptr)           = 0;
    virtual void setIntArray(const std::string& name, const i32* values, u32 count) = 0;
};

class ITexture2D {
public:
    virtual ~ITexture2D() = default;
    virtual void bind(u32 slot = 0) const = 0;
    virtual void unbind()           const = 0;
    virtual u32  getWidth()         const = 0;
    virtual u32  getHeight()        const = 0;
};

class IFramebuffer {
public:
    virtual ~IFramebuffer() = default;
    virtual void bind()             const = 0;
    virtual void unbind()           const = 0;
    virtual void resize(u32 w, u32 h)     = 0;
    virtual u32  getColorAttachment() const = 0;
};

} // namespace ember
