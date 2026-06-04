#pragma once
#include "ember/core/Types.h"
#include "ember/renderer/RHI.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace ember {

struct UVRect { f32 u0 = 0, v0 = 0, u1 = 1, v1 = 1; };

// Subdivides a single texture into named, normalized UV sub-regions.
class TextureAtlas {
public:
    explicit TextureAtlas(std::shared_ptr<ITexture2D> texture) : m_texture(std::move(texture)) {}

    void addRegion(const std::string& name, u32 x, u32 y, u32 w, u32 h) {
        const f32 tw = static_cast<f32>(m_texture->getWidth());
        const f32 th = static_cast<f32>(m_texture->getHeight());
        m_regions[name] = UVRect{
            static_cast<f32>(x) / tw, static_cast<f32>(y) / th,
            static_cast<f32>(x + w) / tw, static_cast<f32>(y + h) / th
        };
    }

    UVRect region(const std::string& name) const {
        auto it = m_regions.find(name);
        return it == m_regions.end() ? UVRect{} : it->second;
    }

    bool has(const std::string& name) const { return m_regions.find(name) != m_regions.end(); }
    const std::shared_ptr<ITexture2D>& texture() const { return m_texture; }

private:
    std::shared_ptr<ITexture2D>             m_texture;
    std::unordered_map<std::string, UVRect> m_regions;
};

} // namespace ember
