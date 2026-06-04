#pragma once
#include "ember/core/Types.h"
#include <vector>

namespace ember {

// Decoded PCM audio (interleaved 32-bit float). Playback is a stub this epic
// (see AudioSystem); this asset just holds the decoded samples.
struct AudioClip {
    std::vector<float> samples;       // interleaved: channels * frames
    u32 channels   = 0;
    u32 sampleRate = 0;
    u64 frameCount = 0;
};

} // namespace ember
