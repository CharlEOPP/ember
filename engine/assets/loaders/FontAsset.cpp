#include "ember/assets/FontAsset.h"
#include "ember/platform/FileSystem.h"
#include "ember/core/Log.h"

#include <stb_truetype.h>   // implementation lives in stb_impl.cpp

#include <vector>

namespace ember {

std::shared_ptr<FontAsset> FontAsset::loadFromFile(const std::string& path, f32 pixelHeight) {
    auto bytes = FileSystem::readBinaryFile(path);
    if (!bytes) {
        EMBER_LOG_ERROR("FontAsset: cannot read '{}': {}", path, bytes.error());
        return nullptr;
    }

    constexpr u32 aw = 512, ah = 512;
    std::vector<unsigned char>     bitmap(static_cast<usize>(aw) * ah);
    std::vector<stbtt_bakedchar>   baked(kCharCount);

    const int rows = stbtt_BakeFontBitmap(
        reinterpret_cast<const unsigned char*>(bytes->data()), 0, pixelHeight,
        bitmap.data(), static_cast<int>(aw), static_cast<int>(ah),
        static_cast<int>(kFirstChar), static_cast<int>(kCharCount), baked.data());
    if (rows == 0) {
        EMBER_LOG_ERROR("FontAsset: failed to bake atlas for '{}' (atlas too small?)", path);
        return nullptr;
    }

    // Expand the single-channel coverage to RGBA white + alpha so the sprite
    // shader can sample it directly. Memory order is R,G,B,A per texel.
    std::vector<u32> rgba(static_cast<usize>(aw) * ah);
    for (usize i = 0; i < rgba.size(); ++i)
        rgba[i] = 0x00FFFFFFu | (static_cast<u32>(bitmap[i]) << 24);

    TextureSpec spec;
    spec.width = aw; spec.height = ah;
    spec.format = TextureFormat::RGBA8;
    spec.filter = TextureFilter::Linear;
    spec.wrap   = TextureWrap::ClampToEdge;

    auto font = std::make_shared<FontAsset>();
    font->m_pixelHeight = pixelHeight;
    font->m_atlasW = aw;
    font->m_atlasH = ah;
    font->m_atlas  = RHI::createTexture2D(spec, rgba.data());
    for (u32 i = 0; i < kCharCount; ++i) {
        const stbtt_bakedchar& b = baked[i];
        font->m_glyphs[i] = { static_cast<f32>(b.x0), static_cast<f32>(b.y0),
                              static_cast<f32>(b.x1), static_cast<f32>(b.y1),
                              b.xoff, b.yoff, b.xadvance };
    }
    return font;
}

bool FontAsset::quad(u32 codepoint, f32& penX, f32& penY,
                     glm::vec2& p0, glm::vec2& p1, glm::vec2& uv0, glm::vec2& uv1) const {
    if (codepoint < kFirstChar || codepoint >= kFirstChar + kCharCount) return false;
    const Glyph& g = m_glyphs[codepoint - kFirstChar];

    p0 = { penX + g.xoff, penY + g.yoff };
    p1 = { p0.x + (g.x1 - g.x0), p0.y + (g.y1 - g.y0) };
    uv0 = { g.x0 / static_cast<f32>(m_atlasW), g.y0 / static_cast<f32>(m_atlasH) };
    uv1 = { g.x1 / static_cast<f32>(m_atlasW), g.y1 / static_cast<f32>(m_atlasH) };
    penX += g.xadvance;
    return true;
}

} // namespace ember
