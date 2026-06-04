#pragma once
#include "ember/core/Types.h"
#include "ember/assets/AudioClip.h"

#include <array>
#include <memory>
#include <mutex>

namespace ember {

enum class AudioBus : u8 { Master, SFX, Music, Count };

struct PlayParams {
    f32      volume = 1.0f;
    f32      pitch  = 1.0f;   // 1 = original rate
    bool     loop   = false;
    AudioBus bus    = AudioBus::SFX;
};

// Opaque, generation-checked reference to one playing voice.
struct AudioHandle {
    u32 index      = 0;
    u32 generation = 0;
    [[nodiscard]] bool valid() const { return generation != 0; }
};

// Software mixer over decoded AudioClips. The mixer itself (renderInto + voice
// management) is always compiled and is headless-testable; the actual output
// device is miniaudio and only exists when EMBER_ENABLE_AUDIO is set — otherwise
// init() returns false and the engine is inert but still mixes on demand.
class AudioEngine {
public:
    static constexpr usize kMaxVoices = 64;

    AudioEngine();      // defined in .cpp (Device is an incomplete pimpl here)
    ~AudioEngine();
    AudioEngine(const AudioEngine&)            = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool init();          // opens the device (EMBER_ENABLE_AUDIO); false if unavailable
    void shutdown();

    void setMasterVolume(f32 v) { m_master = v; }
    void setBusVolume(AudioBus bus, f32 v);

    AudioHandle play(std::shared_ptr<const AudioClip> clip, const PlayParams& params = {});
    void        stop(AudioHandle h);
    void        stopAll();
    void        setVolume(AudioHandle h, f32 v);
    [[nodiscard]] bool isPlaying(AudioHandle h) const;
    [[nodiscard]] usize activeVoices() const;

    // Mix all active voices into `out` (interleaved float, `channels` wide) for
    // `frames` frames. Called by the device callback AND directly by tests.
    void renderInto(f32* out, u64 frames, u32 channels);

private:
    struct Voice {
        std::shared_ptr<const AudioClip> clip;
        f64      cursor = 0.0;     // playback position in source frames
        f32      volume = 1.0f;
        f32      pitch  = 1.0f;
        bool     loop   = false;
        AudioBus bus    = AudioBus::SFX;
        bool     active = false;
        u32      generation = 0;
    };

    f32 gainFor(AudioBus bus) const {
        return (bus == AudioBus::Master ? 1.0f : m_busGain[static_cast<usize>(bus)]) * m_master;
    }

    std::array<Voice, kMaxVoices>                     m_voices{};
    std::array<f32, static_cast<usize>(AudioBus::Count)> m_busGain{{1.0f, 1.0f, 1.0f}};
    f32 m_master  = 1.0f;
    u32 m_nextGen = 1;
    mutable std::mutex m_mutex;

    struct Device;                 // miniaudio device (defined per build config)
    std::unique_ptr<Device> m_device;
};

} // namespace ember
