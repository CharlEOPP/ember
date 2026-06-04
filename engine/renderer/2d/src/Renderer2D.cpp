#include "ember/renderer/Renderer2D.h"
#include "ember/renderer/ShaderLibrary.h"
#include "ember/assets/FontAsset.h"
#include "ember/core/Assert.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <string>
#include <vector>

namespace ember {
namespace {

struct QuadVertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 uv;
    f32       texIndex;
    f32       entityID;
};

constexpr u32 kMaxQuads    = Renderer2D::kMaxQuads;
constexpr u32 kMaxVertices = kMaxQuads * 4;
constexpr u32 kMaxIndices  = kMaxQuads * 6;
constexpr u32 kMaxSlots    = Renderer2D::kMaxTextureSlots;

const char* kSpriteShaderSrc = R"(#type vertex
#version 410 core
layout(location = 0) in vec3  a_Position;
layout(location = 1) in vec4  a_Color;
layout(location = 2) in vec2  a_UV;
layout(location = 3) in float a_TexIndex;
layout(location = 4) in float a_EntityID;
uniform mat4 u_ViewProjection;
out vec4 v_Color; out vec2 v_UV; flat out int v_TexIndex;
void main() {
    v_Color = a_Color; v_UV = a_UV; v_TexIndex = int(a_TexIndex);
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}
#type fragment
#version 410 core
in vec4 v_Color; in vec2 v_UV; flat in int v_TexIndex;
uniform sampler2D u_Textures[16];
out vec4 fragColor;
void main() {
    vec4 s = vec4(1.0);
    switch (v_TexIndex) {
        case  0: s = texture(u_Textures[ 0], v_UV); break;
        case  1: s = texture(u_Textures[ 1], v_UV); break;
        case  2: s = texture(u_Textures[ 2], v_UV); break;
        case  3: s = texture(u_Textures[ 3], v_UV); break;
        case  4: s = texture(u_Textures[ 4], v_UV); break;
        case  5: s = texture(u_Textures[ 5], v_UV); break;
        case  6: s = texture(u_Textures[ 6], v_UV); break;
        case  7: s = texture(u_Textures[ 7], v_UV); break;
        case  8: s = texture(u_Textures[ 8], v_UV); break;
        case  9: s = texture(u_Textures[ 9], v_UV); break;
        case 10: s = texture(u_Textures[10], v_UV); break;
        case 11: s = texture(u_Textures[11], v_UV); break;
        case 12: s = texture(u_Textures[12], v_UV); break;
        case 13: s = texture(u_Textures[13], v_UV); break;
        case 14: s = texture(u_Textures[14], v_UV); break;
        case 15: s = texture(u_Textures[15], v_UV); break;
    }
    fragColor = s * v_Color;
}
)";

struct Renderer2DData {
    std::shared_ptr<IVertexArray>  vao;
    std::shared_ptr<IVertexBuffer> vbo;
    std::shared_ptr<IIndexBuffer>  ibo;
    std::shared_ptr<IShader>       shader;
    std::shared_ptr<ITexture2D>    white;
    std::vector<QuadVertex> vertices;
    u32 quadCount = 0;
    std::array<std::shared_ptr<ITexture2D>, kMaxSlots> slots{};
    u32 slotCount = 1;
    glm::mat4 viewProjection{1.0f};
    glm::vec4 quadCorners[4];
    glm::vec2 quadUVs[4];
    RendererStats stats;
    bool inScene = false;
};
Renderer2DData s;

void startBatch() { s.quadCount = 0; s.slotCount = 1; }

void flush() {
    if (s.quadCount == 0) return;
    s.vbo->setData(s.vertices.data(), s.quadCount * 4 * static_cast<u32>(sizeof(QuadVertex)));
    s.shader->bind();
    s.shader->setMat4("u_ViewProjection", glm::value_ptr(s.viewProjection));
    int samplers[kMaxSlots];
    for (u32 i = 0; i < kMaxSlots; ++i) samplers[i] = static_cast<int>(i);
    s.shader->setIntArray("u_Textures", samplers, kMaxSlots);
    for (u32 i = 0; i < s.slotCount; ++i) s.slots[i]->bind(i);
    s.stats.textureBinds += s.slotCount;
    RHI::setBlend(true);
    RHI::drawIndexed(s.vao, s.quadCount * 6);
    s.stats.drawCalls++;
}

void nextBatch() { flush(); startBatch(); }

u32 resolveSlot(const std::shared_ptr<ITexture2D>& texture) {
    if (!texture) return 0;
    for (u32 i = 1; i < s.slotCount; ++i) if (s.slots[i] == texture) return i;
    if (s.slotCount >= kMaxSlots) nextBatch();
    const u32 slot = s.slotCount;
    s.slots[slot] = texture;
    ++s.slotCount;
    return slot;
}

void submitQuadUV(const glm::vec2& p0, const glm::vec2& p1,
                  const glm::vec2& uv0, const glm::vec2& uv1,
                  const glm::vec4& color, const std::shared_ptr<ITexture2D>& tex) {
    if (s.quadCount >= kMaxQuads) nextBatch();
    const f32 texIndex = static_cast<f32>(resolveSlot(tex));
    const glm::vec2 pos[4] = { {p0.x, p0.y}, {p1.x, p0.y}, {p1.x, p1.y}, {p0.x, p1.y} };
    const glm::vec2 uv [4] = { {uv0.x, uv0.y}, {uv1.x, uv0.y}, {uv1.x, uv1.y}, {uv0.x, uv1.y} };
    QuadVertex* v = &s.vertices[s.quadCount * 4];
    for (int i = 0; i < 4; ++i) {
        v[i].position = glm::vec3(pos[i], 0.0f);
        v[i].color = color; v[i].uv = uv[i]; v[i].texIndex = texIndex; v[i].entityID = -1.0f;
    }
    ++s.quadCount; s.stats.quadCount++; s.stats.vertexCount += 4;
}

u32 nextCodepoint(const std::string& str, usize& i) {
    const auto b0 = static_cast<unsigned char>(str[i]);
    if (b0 < 0x80) { ++i; return b0; }
    int extra = (b0 >= 0xF0) ? 3 : (b0 >= 0xE0) ? 2 : 1;
    u32 cp = b0 & (0x3F >> extra);
    ++i;
    for (int k = 0; k < extra && i < str.size(); ++k, ++i)
        cp = (cp << 6) | (static_cast<unsigned char>(str[i]) & 0x3F);
    return cp;
}

} // namespace

void Renderer2D::init() {
    s.vertices.resize(kMaxVertices);
    s.vbo = RHI::createVertexBuffer(kMaxVertices * static_cast<u32>(sizeof(QuadVertex)));
    s.vbo->setLayout({
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float4, "a_Color"    },
        { ShaderDataType::Float2, "a_UV"       },
        { ShaderDataType::Float,  "a_TexIndex" },
        { ShaderDataType::Float,  "a_EntityID" },
    });
    s.vao = RHI::createVertexArray();
    s.vao->addVertexBuffer(s.vbo);
    std::vector<u32> indices(kMaxIndices);
    u32 offset = 0;
    for (u32 i = 0; i < kMaxIndices; i += 6) {
        indices[i+0]=offset+0; indices[i+1]=offset+1; indices[i+2]=offset+2;
        indices[i+3]=offset+2; indices[i+4]=offset+3; indices[i+5]=offset+0;
        offset += 4;
    }
    s.ibo = RHI::createIndexBuffer(indices.data(), kMaxIndices);
    s.vao->setIndexBuffer(s.ibo);
    s.white = RHI::whiteTexture();
    s.slots[0] = s.white;
    std::string vs, fs;
    ShaderLibrary::splitSource(kSpriteShaderSrc, vs, fs);
    s.shader = RHI::createShader(vs, fs);
    EMBER_VERIFY(s.shader != nullptr, "Renderer2D: failed to compile the sprite shader");
    s.quadCorners[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
    s.quadCorners[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
    s.quadCorners[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
    s.quadCorners[3] = { -0.5f,  0.5f, 0.0f, 1.0f };
    s.quadUVs[0] = { 0.0f, 0.0f }; s.quadUVs[1] = { 1.0f, 0.0f };
    s.quadUVs[2] = { 1.0f, 1.0f }; s.quadUVs[3] = { 0.0f, 1.0f };
}

void Renderer2D::shutdown() { s = Renderer2DData{}; }

void Renderer2D::beginScene(const glm::mat4& viewProjection) {
    s.viewProjection = viewProjection;
    s.stats = RendererStats{};
    s.inScene = true;
    startBatch();
}

void Renderer2D::endScene() { flush(); s.inScene = false; }

void Renderer2D::drawQuad(const glm::mat4& transform, const glm::vec4& color,
                          const std::shared_ptr<ITexture2D>& texture, f32 tiling, f32 entityID) {
    EMBER_ASSERT(s.inScene, "Renderer2D::drawQuad called outside beginScene/endScene");
    if (s.quadCount >= kMaxQuads) nextBatch();
    const f32 texIndex = static_cast<f32>(resolveSlot(texture));
    QuadVertex* v = &s.vertices[s.quadCount * 4];
    for (int i = 0; i < 4; ++i) {
        v[i].position = glm::vec3(transform * s.quadCorners[i]);
        v[i].color = color; v[i].uv = s.quadUVs[i] * tiling; v[i].texIndex = texIndex; v[i].entityID = entityID;
    }
    ++s.quadCount; s.stats.quadCount++; s.stats.vertexCount += 4;
}

void Renderer2D::drawQuad(const glm::mat4& transform, const glm::vec4& color,
                          const std::shared_ptr<ITexture2D>& texture, const glm::vec4& uvRect, f32 entityID) {
    EMBER_ASSERT(s.inScene, "Renderer2D::drawQuad called outside beginScene/endScene");
    if (s.quadCount >= kMaxQuads) nextBatch();
    const f32 texIndex = static_cast<f32>(resolveSlot(texture));
    const glm::vec2 uvMin{uvRect.x, uvRect.y};
    const glm::vec2 uvMax{uvRect.z, uvRect.w};
    QuadVertex* v = &s.vertices[s.quadCount * 4];
    for (int i = 0; i < 4; ++i) {
        v[i].position = glm::vec3(transform * s.quadCorners[i]);
        v[i].color = color;
        v[i].uv = uvMin + s.quadUVs[i] * (uvMax - uvMin);   // map corner across the rect
        v[i].texIndex = texIndex; v[i].entityID = entityID;
    }
    ++s.quadCount; s.stats.quadCount++; s.stats.vertexCount += 4;
}

void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
    drawQuad(t, color, nullptr, 1.0f, -1.0f);
}

void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size,
                          const std::shared_ptr<ITexture2D>& texture, const glm::vec4& tint, f32 tiling) {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
    drawQuad(t, tint, texture, tiling, -1.0f);
}

void Renderer2D::drawSprite(const glm::mat4& worldTransform, const SpriteRenderer& sprite,
                            const std::shared_ptr<ITexture2D>& texture, f32 entityID) {
    const glm::vec3 flip(sprite.flipX ? -1.0f : 1.0f, sprite.flipY ? -1.0f : 1.0f, 1.0f);
    const glm::mat4 t = worldTransform * glm::scale(glm::mat4(1.0f), flip);
    drawQuad(t, sprite.color, texture, sprite.uvRect, entityID);
}

void Renderer2D::drawText(const std::string& text, const FontAsset& font,
                          const glm::vec2& position, f32 scale, const glm::vec4& color) {
    EMBER_ASSERT(s.inScene, "Renderer2D::drawText called outside beginScene/endScene");
    f32 penX = 0.0f, penY = 0.0f;
    for (usize i = 0; i < text.size(); ) {
        const u32 cp = nextCodepoint(text, i);
        if (cp == '\n') { penX = 0.0f; penY += font.pixelHeight(); continue; }
        glm::vec2 gp0, gp1, uv0, uv1;
        if (!font.quad(cp, penX, penY, gp0, gp1, uv0, uv1)) continue;
        const glm::vec2 wp0 = position + scale * glm::vec2(gp0.x, -gp0.y);
        const glm::vec2 wp1 = position + scale * glm::vec2(gp1.x, -gp1.y);
        submitQuadUV(wp0, wp1, uv0, uv1, color, font.atlas());
    }
}

const RendererStats& Renderer2D::stats()  { return s.stats; }
void                 Renderer2D::resetStats() { s.stats = RendererStats{}; }

} // namespace ember
