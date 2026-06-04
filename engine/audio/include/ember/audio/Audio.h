#pragma once
#include "ember/audio/AudioEngine.h"
#include "ember/assets/AssetHandle.h"

#include <string>

namespace ember {
class AssetManager;
struct AudioClip;
}

// Static facade game/script code uses. Bind it once at startup to an AudioEngine
// (and optionally an AssetManager so it can load clips by path). All calls are
// null-safe when unbound.
namespace ember::Audio {

void init(AudioEngine& engine, AssetManager* assets = nullptr);
void shutdown();
AudioEngine* engine();

AudioHandle play(AssetHandle<AudioClip> clip, const PlayParams& params = {});
AudioHandle play(const std::string& virtualPath, const PlayParams& params = {});
AudioHandle playOneShot(AssetHandle<AudioClip> clip, f32 volume = 1.0f);

void stop(AudioHandle h);
void stopAll();

} // namespace ember::Audio
