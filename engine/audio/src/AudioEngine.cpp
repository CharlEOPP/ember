#include "ember/audio/AudioEngine.h"

#include <cmath>

namespace ember {

// ---------------------------------------------------------------------------
// Mixer + voice management — always compiled, miniaudio-free, headless-testable.
// ---------------------------------------------------------------------------

void AudioEngine::setBusVolume(AudioBus bus, f32 v) {
    if (bus == AudioBus::Master) { m_master = v; return; }
    m_busGain[static_cast<usize>(bus)] = v;
}

AudioHandle AudioEngine::play(std::shared_ptr<const AudioClip> clip, const PlayParams& params) {
    if (!clip || clip->frameCount == 0 || clip->channels == 0) return {};
    std::lock_guard<std::mutex> lk(m_mutex);
    for (u32 i = 0; i < kMaxVoices; ++i) {
        Voice& v = m_voices[i];
        if (v.active) continue;
        v.clip = std::move(clip);
        v.cursor = 0.0;
        v.volume = params.volume;
        v.pitch  = params.pitch > 0.0f ? params.pitch : 1.0f;
        v.loop   = params.loop;
        v.bus    = params.bus;
        v.active = true;
        v.generation = m_nextGen++;
        if (m_nextGen == 0) m_nextGen = 1;
        return AudioHandle{ i, v.generation };
    }
    return {};   // pool full
}

void AudioEngine::stop(AudioHandle h) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (h.index >= kMaxVoices) return;
    Voice& v = m_voices[h.index];
    if (v.generation == h.generation && v.active) { v.active = false; v.clip.reset(); }
}

void AudioEngine::stopAll() {
    std::lock_guard<std::mutex> lk(m_mutex);
    for (Voice& v : m_voices) { v.active = false; v.clip.reset(); }
}

void AudioEngine::setVolume(AudioHandle h, f32 vol) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (h.index >= kMaxVoices) return;
    Voice& v = m_voices[h.index];
    if (v.generation == h.generation && v.active) v.volume = vol;
}

bool AudioEngine::isPlaying(AudioHandle h) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (h.index >= kMaxVoices) return false;
    const Voice& v = m_voices[h.index];
    return v.active && v.generation == h.generation;
}

usize AudioEngine::activeVoices() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    usize n = 0;
    for (const Voice& v : m_voices) if (v.active) ++n;
    return n;
}

void AudioEngine::renderInto(f32* out, u64 frames, u32 channels) {
    for (u64 i = 0; i < frames * channels; ++i) out[i] = 0.0f;
    std::lock_guard<std::mutex> lk(m_mutex);
    for (Voice& v : m_voices) {
        if (!v.active || !v.clip) continue;
        const AudioClip& c = *v.clip;
        const f32 gain = v.volume * gainFor(v.bus);
        for (u64 f = 0; f < frames; ++f) {
            if (v.cursor >= static_cast<f64>(c.frameCount)) {
                if (v.loop) v.cursor = std::fmod(v.cursor, static_cast<f64>(c.frameCount));
                else { v.active = false; v.clip.reset(); break; }
            }
            const u64 i0 = static_cast<u64>(v.cursor);
            u64 i1 = i0 + 1;
            const f64 frac = v.cursor - static_cast<f64>(i0);
            if (i1 >= c.frameCount) i1 = v.loop ? 0 : i0;
            for (u32 ch = 0; ch < channels; ++ch) {
                const u32 cc = (c.channels == 1) ? 0u : (ch % c.channels);
                const f32 s0 = c.samples[i0 * c.channels + cc];
                const f32 s1 = c.samples[i1 * c.channels + cc];
                out[f * channels + ch] += static_cast<f32>(s0 + (s1 - s0) * frac) * gain;
            }
            v.cursor += v.pitch;
        }
    }
}

} // namespace ember

// ---------------------------------------------------------------------------
// Output device — miniaudio. Only present when EMBER_ENABLE_AUDIO is set.
// ---------------------------------------------------------------------------
#if defined(EMBER_ENABLE_AUDIO)
#include "ember/core/Log.h"
#include <miniaudio.h>

namespace ember {

struct AudioEngine::Device { ma_device dev; bool started = false; };

static void ember_audio_data_callback(ma_device* d, void* output, const void*, ma_uint32 frameCount) {
    auto* engine = static_cast<AudioEngine*>(d->pUserData);
    engine->renderInto(static_cast<f32*>(output), frameCount, d->playback.channels);
}

bool AudioEngine::init() {
    m_device = std::make_unique<Device>();
    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate        = 48000;
    cfg.dataCallback      = ember_audio_data_callback;
    cfg.pUserData         = this;
    if (ma_device_init(nullptr, &cfg, &m_device->dev) != MA_SUCCESS) {
        EMBER_LOG_WARN("AudioEngine: no playback device available; audio inert");
        m_device.reset();
        return false;
    }
    if (ma_device_start(&m_device->dev) != MA_SUCCESS) {
        ma_device_uninit(&m_device->dev);
        m_device.reset();
        return false;
    }
    m_device->started = true;
    EMBER_LOG_INFO("AudioEngine: device started ({} ch, {} Hz)",
                   m_device->dev.playback.channels, m_device->dev.sampleRate);
    return true;
}

AudioEngine::AudioEngine() = default;

void AudioEngine::shutdown() {
    if (m_device && m_device->started) ma_device_uninit(&m_device->dev);
    m_device.reset();
    stopAll();
}

AudioEngine::~AudioEngine() { shutdown(); }

} // namespace ember

#else   // audio disabled: inert device, mixer still usable

namespace ember {
struct AudioEngine::Device {};
AudioEngine::AudioEngine()    = default;
bool AudioEngine::init()      { return false; }
void AudioEngine::shutdown()  { stopAll(); }
AudioEngine::~AudioEngine()   { shutdown(); }
} // namespace ember

#endif
