#include "ember/renderer/DebugDraw.h"

// Entire implementation is Debug-only; in Release the header provides no-ops and
// this translation unit is empty (no GL resources created).
#if !defined(NDEBUG)

#include "ember/renderer/RHI.h"
#include "ember/renderer/ShaderLibrary.h"
#include "ember/core/Assert.h"

#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <vector>

namespace ember::DebugDraw {

namespace {

struct LineVertex { glm::vec3 position; glm::vec4 color; };

constexpr u32 kMaxLines    = 8192;
constexpr u32 kMaxVertices = kMaxLines * 2;

const char* kLineShaderSrc = R"(#type vertex
#version 410 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
uniform mat4 u_ViewProjection;
out vec4 v_Color;
void main() { v_Color = a_Color; gl_Position = u_ViewProjection * vec4(a_Position, 1.0); }
#type fragment
#version 410 core
in vec4 v_Color;
out vec4 fragColor;
void main() { fragColor = v_Color; }
)";

struct Data {
    std::shared_ptr<IVertexArray>  vao;
    std::shared_ptr<IVertexBuffer> vbo;
    std::shared_ptr<IShader>       shader;
    std::vector<LineVertex>        vertices;
    glm::mat4 viewProjection{1.0f};
    bool initialized = false;
};
Data s;

} // namespace

void init() {
    s.vertices.reserve(kMaxVertices);
    s.vbo = RHI::createVertexBuffer(kMaxVertices * static_cast<u32>(sizeof(LineVertex)));
    s.vbo->setLayout({ { ShaderDataType::Float3, "a_Position" }, { ShaderDataType::Float4, "a_Color" } });
    s.vao = RHI::createVertexArray();
    s.vao->addVertexBuffer(s.vbo);

    std::string vs, fs;
    ShaderLibrary::splitSource(kLineShaderSrc, vs, fs);
    s.shader = RHI::createShader(vs, fs);
    EMBER_VERIFY(s.shader != nullptr, "DebugDraw: failed to compile the line shader");
    s.initialized = true;
}

void shutdown() { s = Data{}; }

void begin(const glm::mat4& viewProjection) {
    s.viewProjection = viewProjection;
    s.vertices.clear();
}

void line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& color) {
    if (s.vertices.size() + 2 > kMaxVertices) return;
    s.vertices.push_back({ glm::vec3(a, 0.0f), color });
    s.vertices.push_back({ glm::vec3(b, 0.0f), color });
}

void rect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
    const glm::vec2 h = size * 0.5f;
    const glm::vec2 p0 = position + glm::vec2(-h.x, -h.y);
    const glm::vec2 p1 = position + glm::vec2( h.x, -h.y);
    const glm::vec2 p2 = position + glm::vec2( h.x,  h.y);
    const glm::vec2 p3 = position + glm::vec2(-h.x,  h.y);
    line(p0, p1, color); line(p1, p2, color); line(p2, p3, color); line(p3, p0, color);
}

void circle(const glm::vec2& center, f32 radius, const glm::vec4& color, int segments) {
    if (segments < 3) segments = 3;
    const f32 step = 6.2831853f / static_cast<f32>(segments);
    glm::vec2 prev = center + glm::vec2(radius, 0.0f);
    for (int i = 1; i <= segments; ++i) {
        const f32 a = step * static_cast<f32>(i);
        glm::vec2 cur = center + glm::vec2(std::cos(a) * radius, std::sin(a) * radius);
        line(prev, cur, color);
        prev = cur;
    }
}

void flush() {
    if (!s.initialized || s.vertices.empty()) return;
    s.vbo->setData(s.vertices.data(), static_cast<u32>(s.vertices.size() * sizeof(LineVertex)));
    s.shader->bind();
    s.shader->setMat4("u_ViewProjection", glm::value_ptr(s.viewProjection));
    RHI::drawLines(s.vao, static_cast<u32>(s.vertices.size()));
    s.vertices.clear();
}

} // namespace ember::DebugDraw

#endif // !NDEBUG
