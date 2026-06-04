#pragma once
#include "ember/core/Types.h"
#include "ember/renderer/RHI.h"

#include <glm/glm.hpp>
#include <array>
#include <memory>
#include <string>

namespace ember {

// A bitmap font atlas baked from a TTF via stb_truetype. The atlas is RGBA
// (white with per-texel coverage in alpha) so glyphs render through the normal
// sprite shader, tinted by the text color.
class FontAsset {
public:
    static constexpr u32 kFirstChar = 32;    // space
    static constexpr u32 kCharCount = 95;    // through '~' (126)

    static std::shared_ptr<FontAsset> loadFromFile(const std::string& path, f32 pixelHeight);

    const std::shared_ptr<ITexture2D>& atlas() const { return m_atlas; }
    f32 pixelHeight() const { return m_pixelHeight; }

    // Emit the quad for a codepoint (pixel-space, relative to the pen) and advance
    // the pen. Returns false if the codepoint has no glyph in the baked range.
    bool quad(u32 codepoint, f32& penX, f32& penY,
              glm::vec2& p0, glm::vec2& p1, glm::vec2& uv0, glm::vec2& uv1) const;

private:
    struct Glyph { f32 x0, y0, x1, y1; f32 xoff, yoff, xadvance; };

    std::shared_ptr<ITexture2D>      m_atlas;
    std::array<Glyph, kCharCount>    m_glyphs{};
    f32 m_pixelHeight = 0.0f;
    u32 m_atlasW = 0, m_atlasH = 0;
};

} // namespace ember
