#include "ember/renderer/Texture2DLoader.h"
#include "ember/core/Log.h"

#include <stb_image.h>   // implementation lives in stb_impl.cpp

#include <array>

namespace ember {

std::shared_ptr<ITexture2D> Texture2DLoader::missingTexture() {
    static std::shared_ptr<ITexture2D> s_missing;
    if (!s_missing) {
        // 2x2 magenta / black checker so missing assets are obvious.
        const u32 m = 0xFFFF00FFu;  // magenta (ABGR in memory: R,G,B,A bytes)
        const u32 k = 0xFF000000u;  // black
        const std::array<u32, 4> pixels = { m, k, k, m };
        TextureSpec spec;
        spec.width = 2; spec.height = 2;
        spec.format = TextureFormat::RGBA8;
        spec.filter = TextureFilter::Nearest;
        s_missing = RHI::createTexture2D(spec, pixels.data());
    }
    return s_missing;
}

std::shared_ptr<ITexture2D> Texture2DLoader::load(const std::string& path) {
    stbi_set_flip_vertically_on_load(1);   // match the engine's bottom-left UV origin

    int width = 0, height = 0, channels = 0;
    stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        EMBER_LOG_ERROR("Texture2DLoader: failed to load '{}': {}", path, stbi_failure_reason());
        return missingTexture();
    }

    TextureSpec spec;
    spec.width  = static_cast<u32>(width);
    spec.height = static_cast<u32>(height);
    spec.format = (channels == 4) ? TextureFormat::RGBA8
                : (channels == 3) ? TextureFormat::RGB8
                                  : TextureFormat::R8;

    auto texture = RHI::createTexture2D(spec, data);
    stbi_image_free(data);
    return texture;
}

} // namespace ember
