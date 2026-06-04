#include "ember/assets/loaders/FontLoader.h"
#include <cstdlib>

namespace ember {

std::shared_ptr<FontAsset> FontLoader::load(const std::filesystem::path& path,
                                            const AssetSettings& settings) {
    const std::string ph = settings.get("pixelHeight", "48");
    f32 pixelHeight = static_cast<f32>(std::strtod(ph.c_str(), nullptr));
    if (pixelHeight <= 0.0f) pixelHeight = 48.0f;
    return FontAsset::loadFromFile(path.string(), pixelHeight);
}

} // namespace ember
