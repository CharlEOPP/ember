#include "ember/audio/Audio.h"
#include "ember/assets/AssetManager.h"
#include "ember/assets/AudioClip.h"

namespace ember::Audio {

namespace {
AudioEngine*  g_engine = nullptr;
AssetManager* g_assets = nullptr;
}

void init(AudioEngine& engine, AssetManager* assets) { g_engine = &engine; g_assets = assets; }
void shutdown() { g_engine = nullptr; g_assets = nullptr; }
AudioEngine* engine() { return g_engine; }

AudioHandle play(AssetHandle<AudioClip> clip, const PlayParams& params) {
    if (!g_engine || !g_assets) return {};
    std::shared_ptr<AudioClip> c = g_assets->getShared<AudioClip>(clip);
    return c ? g_engine->play(c, params) : AudioHandle{};
}

AudioHandle play(const std::string& virtualPath, const PlayParams& params) {
    if (!g_engine || !g_assets) return {};
    return play(g_assets->load<AudioClip>(virtualPath), params);
}

AudioHandle playOneShot(AssetHandle<AudioClip> clip, f32 volume) {
    PlayParams p; p.volume = volume; p.loop = false; p.bus = AudioBus::SFX;
    return play(clip, p);
}

void stop(AudioHandle h) { if (g_engine) g_engine->stop(h); }
void stopAll()           { if (g_engine) g_engine->stopAll(); }

} // namespace ember::Audio
