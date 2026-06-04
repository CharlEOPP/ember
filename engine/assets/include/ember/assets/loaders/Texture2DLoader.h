#pragma once
#include "ember/assets/IAssetLoader.h"
#include "ember/assets/Texture2D.h"
#include <memory>

namespace ember {

// Loads images (PNG/JPG/BMP) via stb_image into an RHI texture wrapped in a
// Texture2D asset. Never returns null — falls back to the magenta "missing
// texture" placeholder so a bad path is visible rather than fatal.
class Texture2DLoader : public IAssetLoader<Texture2D> {
public:
    std::shared_ptr<Texture2D> load(const std::filesystem::path& path,
                                    const AssetSettings& settings) override;

    // Async two-phase: decode on a worker, GL upload on the main thread.
    bool supportsAsync() const override { return true; }
    std::shared_ptr<void>      loadCPU(const std::filesystem::path& path,
                                       const AssetSettings& settings) override;
    std::shared_ptr<Texture2D> finalize(const std::shared_ptr<void>& cpuPayload) override;

    // Shared 2x2 magenta/black placeholder (raw RHI texture, for renderer use).
    static std::shared_ptr<ITexture2D> missingTexture();
};

} // namespace ember
