#pragma once
#include "ember/assets/IAssetLoader.h"
#include "ember/assets/AudioClip.h"
#include <memory>

namespace ember {

// Decodes an audio file (WAV/MP3/FLAC/OGG via miniaudio) to interleaved float
// PCM. Only built when EMBER_ENABLE_AUDIO is set. The header is miniaudio-free;
// the implementation TU is the only place miniaudio appears.
class AudioClipLoader : public IAssetLoader<AudioClip> {
public:
    std::shared_ptr<AudioClip> load(const std::filesystem::path& path,
                                    const AssetSettings& settings) override;
};

} // namespace ember
