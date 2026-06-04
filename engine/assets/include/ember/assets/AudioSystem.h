#pragma once
#include "ember/core/Types.h"
#include "ember/assets/AssetHandle.h"

namespace ember {

struct AudioClip;

// Stub audio system — playback is deferred (see 05/06 notes). The API exists so
// game code can compile against it; methods are no-ops for now. A real backend
// (miniaudio device + mixer) lands in a later epic.
class AudioSystem {
public:
    void init()     {}
    void shutdown() {}
    void play(AssetHandle<AudioClip> /*clip*/, f32 /*volume*/ = 1.0f) {}
    void stopAll()  {}
};

} // namespace ember
