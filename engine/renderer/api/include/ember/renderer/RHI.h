#pragma once
#include "ember/core/Types.h"
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>
namespace ember {
class IVertexBuffer; class IIndexBuffer; class IVertexArray;
class IShader; class ITexture2D; class IFramebuffer;
enum class RHIBackend  { OpenGL };
enum class BufferUsage { Static, Dynamic, Stream };
enum class ShaderDataType { None=0, Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, Mat3, Mat4, Bool };
u32 shaderDataTypeSize(ShaderDataType type);
u32 shaderDataTypeComponentCount(ShaderDataType type);
struct BufferElement {
    std::string name; ShaderDataType type=ShaderDataType::None; bool normalized=false; u32 size=0; u32 offset=0;
    BufferElement()=default;
    BufferElement(ShaderDataType t, std::string n, bool norm=false): name(std::move(n)),type(t),normalized(norm),size(shaderDataTypeSize(t)){}
};
class BufferLayout {
public:
    BufferLayout()=default;
    BufferLayout(std::initializer_list<BufferElement> elements);
    u32 stride() const { return m_stride; }
    const std::vector<BufferElement>& elements() const { return m_elements; }
    bool empty() const { return m_elements.empty(); }
private:
    std::vector<BufferElement> m_elements; u32 m_stride=0;
};
enum class TextureFormat { RGBA8, RGB8, R8 };
enum class TextureFilter { Nearest, Linear };
enum class TextureWrap   { Repeat, ClampToEdge };
struct TextureSpec {
    u32 width=1, height=1;
    TextureFormat format=TextureFormat::RGBA8;
    TextureFilter filter=TextureFilter::Linear;
    TextureWrap wrap=TextureWrap::Repeat;
    bool generateMips=false;
};
struct FramebufferSpec { u32 width=1, height=1; bool depth=true; };
class RHI {
public:
    static void init(RHIBackend backend);
    static void shutdown();
    static std::shared_ptr<IVertexBuffer> createVertexBuffer(const void* data, u32 size, BufferUsage usage=BufferUsage::Static);
    static std::shared_ptr<IVertexBuffer> createVertexBuffer(u32 sizeBytes);
    static std::shared_ptr<IIndexBuffer>  createIndexBuffer(const u32* indices, u32 count);
    static std::shared_ptr<IVertexArray>  createVertexArray();
    static std::shared_ptr<IShader>       createShader(const std::string& vertSrc, const std::string& fragSrc);
    static std::shared_ptr<ITexture2D>    createTexture2D(const TextureSpec& spec, const void* pixels=nullptr);
    static std::shared_ptr<IFramebuffer>  createFramebuffer(const FramebufferSpec& spec);
    static std::shared_ptr<ITexture2D>    whiteTexture();
    static void setViewport(u32 x, u32 y, u32 width, u32 height);
    static void setClearColor(f32 r, f32 g, f32 b, f32 a=1.0f);
    static void clear();
    static void setBlend(bool enabled);
    static void drawIndexed(const std::shared_ptr<IVertexArray>& vao, u32 indexCount=0);
    static void drawIndexed(const std::shared_ptr<IVertexBuffer>& vbo, const std::shared_ptr<IIndexBuffer>& ibo, u32 indexCount);
    static void drawLines(const std::shared_ptr<IVertexArray>& vao, u32 vertexCount);
private:
    RHI()=delete;
};
class IVertexBuffer {
public:
    virtual ~IVertexBuffer()=default;
    virtual void bind() const=0; virtual void unbind() const=0;
    virtual void setData(const void* data, u32 size)=0;
    virtual u32 getSize() const=0;
    virtual void setLayout(const BufferLayout& layout)=0;
    virtual const BufferLayout& getLayout() const=0;
};
class IIndexBuffer {
public:
    virtual ~IIndexBuffer()=default;
    virtual void bind() const=0; virtual void unbind() const=0;
    virtual u32 getCount() const=0;
};
class IVertexArray {
public:
    virtual ~IVertexArray()=default;
    virtual void bind() const=0; virtual void unbind() const=0;
    virtual void addVertexBuffer(const std::shared_ptr<IVertexBuffer>& vbo)=0;
    virtual void setIndexBuffer(const std::shared_ptr<IIndexBuffer>& ibo)=0;
    virtual u32 indexCount() const=0;
};
class IShader {
public:
    virtual ~IShader()=default;
    virtual void bind() const=0; virtual void unbind() const=0;
    virtual void setInt(const std::string& name, i32 value)=0;
    virtual void setFloat(const std::string& name, f32 value)=0;
    virtual void setVec2(const std::string& name, f32 x, f32 y)=0;
    virtual void setVec3(const std::string& name, f32 x, f32 y, f32 z)=0;
    virtual void setVec4(const std::string& name, f32 x, f32 y, f32 z, f32 w)=0;
    virtual void setMat3(const std::string& name, const f32* mat3ptr)=0;
    virtual void setMat4(const std::string& name, const f32* mat4ptr)=0;
    virtual void setIntArray(const std::string& name, const i32* values, u32 count)=0;
};
class ITexture2D {
public:
    virtual ~ITexture2D()=default;
    virtual void bind(u32 slot=0) const=0; virtual void unbind() const=0;
    virtual u32 getWidth() const=0; virtual u32 getHeight() const=0;
    virtual u32 rendererID() const=0;
};
class IFramebuffer {
public:
    virtual ~IFramebuffer()=default;
    virtual void bind() const=0; virtual void unbind() const=0;
    virtual void resize(u32 w, u32 h)=0;
    virtual u32 getColorAttachment() const=0;
};
} // namespace ember
