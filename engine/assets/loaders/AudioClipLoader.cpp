#include "ember/assets/loaders/AudioClipLoader.h"
#include "ember/core/Log.h"

#include <miniaudio.h>   // implementation compiled in miniaudio_impl.cpp

namespace ember {

std::shared_ptr<AudioClip> AudioClipLoader::load(const std::filesystem::path& path,
                                                 const AssetSettings& /*settings*/) {
    // Decode to interleaved 32-bit float, keeping the file's channel count / rate.
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    ma_decoder decoder;
    if (ma_decoder_init_file(path.string().c_str(), &config, &decoder) != MA_SUCCESS) {
        EMBER_LOG_ERROR("AudioClipLoader: failed to open '{}'", path.string());
        return nullptr;
    }

    ma_uint64 frames = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frames);

    auto clip = std::make_shared<AudioClip>();
    clip->channels   = decoder.outputChannels;
    clip->sampleRate = decoder.outputSampleRate;
    clip->samples.resize(static_cast<usize>(frames) * decoder.outputChannels);

    ma_uint64 read = 0;
    ma_decoder_read_pcm_frames(&decoder, clip->samples.data(), frames, &read);
    clip->frameCount = read;
    clip->samples.resize(static_cast<usize>(read) * decoder.outputChannels);

    ma_decoder_uninit(&decoder);
    EMBER_LOG_INFO("AudioClipLoader: '{}' — {} frames, {} ch, {} Hz",
                   path.string(), clip->frameCount, clip->channels, clip->sampleRate);
    return clip;
}

} // namespace ember
