#pragma once
#include "ember/assets/IAssetLoader.h"   // AssetSettings
#include "ember/renderer/RHI.h"          // TextureFormat / Filter / Wrap

#include <string>
#include <vector>

namespace ember {

// Typed view over a texture's `.meta` AssetSettings (the import pipeline + the
// editor edit these; Texture2DLoader reads the same keys). ImGui/GL-free.
struct TextureImportSettings {
    TextureFilter filter       = TextureFilter::Linear;
    TextureWrap   wrap         = TextureWrap::Repeat;
    TextureFormat format       = TextureFormat::RGBA8;
    bool          generateMips = false;
    int           maxSize      = 0;   // 0 = no limit

    static TextureImportSettings fromSettings(const AssetSettings& s);
    [[nodiscard]] AssetSettings  toSettings() const;
};

// Typed view over a font's `.meta` settings: the atlas sizes to rasterize.
struct FontImportSettings {
    std::vector<int> sizes = {12, 14, 16, 24, 32, 48};

    static FontImportSettings fromSettings(const AssetSettings& s);
    [[nodiscard]] AssetSettings toSettings() const;
};

} // namespace ember
