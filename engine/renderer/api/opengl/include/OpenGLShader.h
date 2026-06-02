#pragma once
#include "ember/renderer/RHI.h"
#include <string>
#include <unordered_map>

namespace ember {

class OpenGLShader : public IShader {
public:
    OpenGLShader(const std::string& vertSrc, const std::string& fragSrc);
    ~OpenGLShader() override;

    void bind()   const override;
    void unbind() const override;

    void setInt     (const std::string& name, i32 value)                     override;
    void setFloat   (const std::string& name, f32 value)                     override;
    void setVec2    (const std::string& name, f32 x, f32 y)                  override;
    void setVec3    (const std::string& name, f32 x, f32 y, f32 z)           override;
    void setVec4    (const std::string& name, f32 x, f32 y, f32 z, f32 w)    override;
    void setMat4    (const std::string& name, const f32* mat4ptr)             override;
    void setIntArray(const std::string& name, const i32* values, u32 count)  override;

    bool isValid() const { return m_rendererID != 0; }

private:
    i32 getUniformLocation(const std::string& name);

    u32 m_rendererID = 0;
    std::unordered_map<std::string, i32> m_uniformCache;
};

} // namespace ember
