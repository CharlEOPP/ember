#include "OpenGLShader.h"
#include "ember/core/Log.h"
#include <glad/gl.h>
#include <vector>

namespace ember {

static u32 compileShader(GLenum type, const std::string& src) {
    const u32 shader = glCreateShader(type);
    const char* cstr = src.c_str();
    glShaderSource(shader, 1, &cstr, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<std::size_t>(len));
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        EMBER_LOG_ERROR("Shader compilation failed:\n{}", log.data());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

OpenGLShader::OpenGLShader(const std::string& vertSrc, const std::string& fragSrc) {
    const u32 vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    const u32 frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    if (!vert || !frag) {
        glDeleteShader(vert);
        glDeleteShader(frag);
        return; // m_rendererID stays 0 — isValid() returns false
    }

    m_rendererID = glCreateProgram();
    glAttachShader(m_rendererID, vert);
    glAttachShader(m_rendererID, frag);
    glLinkProgram(m_rendererID);

    GLint ok = 0;
    glGetProgramiv(m_rendererID, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(m_rendererID, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<std::size_t>(len));
        glGetProgramInfoLog(m_rendererID, len, nullptr, log.data());
        EMBER_LOG_ERROR("Shader link failed:\n{}", log.data());
        glDeleteProgram(m_rendererID);
        m_rendererID = 0;
    }

    glDetachShader(m_rendererID, vert);
    glDetachShader(m_rendererID, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
}

OpenGLShader::~OpenGLShader() {
    if (m_rendererID) glDeleteProgram(m_rendererID);
}

void OpenGLShader::bind()   const { glUseProgram(m_rendererID); }
void OpenGLShader::unbind() const { glUseProgram(0); }

i32 OpenGLShader::getUniformLocation(const std::string& name) {
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) return it->second;
    const i32 loc = glGetUniformLocation(m_rendererID, name.c_str());
    if (loc == -1)
        EMBER_LOG_WARN("Uniform '{}' not found in shader", name);
    m_uniformCache[name] = loc;
    return loc;
}

void OpenGLShader::setInt(const std::string& name, i32 value) {
    glUniform1i(getUniformLocation(name), value);
}
void OpenGLShader::setFloat(const std::string& name, f32 value) {
    glUniform1f(getUniformLocation(name), value);
}
void OpenGLShader::setVec2(const std::string& name, f32 x, f32 y) {
    const f32 v[] = {x, y};
    glUniform2fv(getUniformLocation(name), 1, v);
}
void OpenGLShader::setVec3(const std::string& name, f32 x, f32 y, f32 z) {
    const f32 v[] = {x, y, z};
    glUniform3fv(getUniformLocation(name), 1, v);
}
void OpenGLShader::setVec4(const std::string& name, f32 x, f32 y, f32 z, f32 w) {
    const f32 v[] = {x, y, z, w};
    glUniform4fv(getUniformLocation(name), 1, v);
}
void OpenGLShader::setMat3(const std::string& name, const f32* ptr) {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, ptr);
}
void OpenGLShader::setMat4(const std::string& name, const f32* ptr) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, ptr);
}
void OpenGLShader::setIntArray(const std::string& name, const i32* values, u32 count) {
    glUniform1iv(getUniformLocation(name), static_cast<GLsizei>(count), values);
}

} // namespace ember
