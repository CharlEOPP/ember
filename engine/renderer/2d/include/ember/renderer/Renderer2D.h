#pragma once
#include "ember/core/Types.h"
#include "ember/renderer/RHI.h"
#include "ember/renderer/Components2D.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
namespace ember {
class FontAsset;
struct RendererStats {
    u32 drawCalls=0; u32 quadCount=0; u32 vertexCount=0; u32 textureBinds=0;
};
class Renderer2D {
public:
    static void init();
    static void shutdown();
    static void beginScene(const glm::mat4& viewProjection);
    static void endScene();
    static void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    static void drawQuad(const glm::vec2& position, const glm::vec2& size,
                         const std::shared_ptr<ITexture2D>& texture,
                         const glm::vec4& tint = glm::vec4(1.0f), f32 tiling = 1.0f);
    static void drawQuad(const glm::mat4& transform, const glm::vec4& color,
                         const std::shared_ptr<ITexture2D>& texture = nullptr,
                         f32 tiling = 1.0f, f32 entityID = -1.0f);
    // UV-rect overload: maps the quad across (minU,minV,maxU,maxV) — used for
    // atlas frames (sprite animation, tilemaps, particles).
    static void drawQuad(const glm::mat4& transform, const glm::vec4& color,
                         const std::shared_ptr<ITexture2D>& texture,
                         const glm::vec4& uvRect, f32 entityID = -1.0f);
    // `texture` is the resolved RHI texture for the sprite's AssetHandle
    // (SpriteRenderSystem resolves it via the AssetManager). May be null.
    static void drawSprite(const glm::mat4& worldTransform, const SpriteRenderer& sprite,
                           const std::shared_ptr<ITexture2D>& texture, f32 entityID = -1.0f);
    static void drawText(const std::string& text, const FontAsset& font,
                         const glm::vec2& position, f32 scale, const glm::vec4& color);
    static const RendererStats& stats();
    static void resetStats();
    static constexpr u32 kMaxQuads        = 4096;
    static constexpr u32 kMaxTextureSlots = 16;
};
} // namespace ember
