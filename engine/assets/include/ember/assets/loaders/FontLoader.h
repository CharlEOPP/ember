#pragma once
#include "ember/assets/IAssetLoader.h"
#include "ember/assets/FontAsset.h"
#include <memory>

namespace ember {

// Bakes a TTF into a FontAsset atlas via stb_truetype. `pixelHeight` is read
// from the .meta setting of the same name (default 48).
class FontLoader : public IAssetLoader<FontAsset> {
public:
    std::shared_ptr<FontAsset> load(const std::filesystem::path& path,
                                    const AssetSettings& settings) override;
};

} // namespace ember
