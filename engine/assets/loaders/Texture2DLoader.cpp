#include "ember/assets/loaders/Texture2DLoader.h"
#include "ember/core/Log.h"

#include <stb_image.h>   // implementation lives in stb_impl.cpp

#include <array>
#include <vector>

namespace ember {

namespace {
TextureFilter parseFilter(const AssetSettings& s) {
    return s.get("filter", "Linear") == "Nearest" ? TextureFilter::Nearest : TextureFilter::Linear;
}
TextureWrap parseWrap(const AssetSettings& s) {
    return s.get("wrap", "Repeat") == "ClampToEdge" ? TextureWrap::ClampToEdge : TextureWrap::Repeat;
}

// CPU-decoded image, produced on a worker thread and finalized (GL upload) on
// the main thread.
struct CpuImage {
    std::vector<unsigned char> pixels;
    int  w = 0, h = 0, channels = 0;
    TextureFilter filter = TextureFilter::Linear;
    TextureWrap   wrap   = TextureWrap::Repeat;
};

std::shared_ptr<CpuImage> decode(const std::filesystem::path& path, const AssetSettings& settings) {
    stbi_set_flip_vertically_on_load(1);   // match the engine's bottom-left UV origin
    int w = 0, h = 0, channels = 0;
    stbi_uc* data = stbi_load(path.string().c_str(), &w, &h, &channels, 0);
    if (!data) {
        EMBER_LOG_ERROR("Texture2DLoader: failed to load '{}': {}", path.string(), stbi_failure_reason());
        return nullptr;
    }
    auto img = std::make_shared<CpuImage>();
    img->w = w; img->h = h; img->channels = channels;
    img->filter = parseFilter(settings);
    img->wrap   = parseWrap(settings);
    img->pixels.assign(data, data + static_cast<usize>(w) * h * channels);
    stbi_image_free(data);
    return img;
}

std::shared_ptr<Texture2D> upload(const CpuImage& img) {
    TextureSpec spec;
    spec.width  = static_cast<u32>(img.w);
    spec.height = static_cast<u32>(img.h);
    spec.format = (img.channels == 4) ? TextureFormat::RGBA8
                : (img.channels == 3) ? TextureFormat::RGB8
                                      : TextureFormat::R8;
    spec.filter = img.filter;
    spec.wrap   = img.wrap;
    auto asset = std::make_shared<Texture2D>();
    asset->texture = RHI::createTexture2D(spec, img.pixels.data());
    asset->width   = spec.width;
    asset->height  = spec.height;
    return asset;
}

std::shared_ptr<Texture2D> missingAsset() {
    auto fallback = std::make_shared<Texture2D>();
    fallback->texture = Texture2DLoader::missingTexture();
    fallback->width = 2; fallback->height = 2;
    return fallback;
}
} // namespace

std::shared_ptr<ITexture2D> Texture2DLoader::missingTexture() {
    static std::shared_ptr<ITexture2D> s_missing;
    if (!s_missing) {
        const u32 m = 0xFFFF00FFu;  // magenta
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

std::shared_ptr<Texture2D> Texture2DLoader::load(const std::filesystem::path& path,
                                                 const AssetSettings& settings) {
    auto img = decode(path, settings);
    return img ? upload(*img) : missingAsset();
}

// ---- async two-phase ----
std::shared_ptr<void> Texture2DLoader::loadCPU(const std::filesystem::path& path,
                                               const AssetSettings& settings) {
    return decode(path, settings);   // worker thread: file I/O + stb decode
}

std::shared_ptr<Texture2D> Texture2DLoader::finalize(const std::shared_ptr<void>& cpuPayload) {
    if (!cpuPayload) return missingAsset();
    return upload(*std::static_pointer_cast<CpuImage>(cpuPayload));   // main thread: GL upload
}

} // namespace ember
