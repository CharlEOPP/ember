#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ember/audio/AudioEngine.h"
#include "ember/assets/AudioClip.h"

#include <memory>
#include <vector>

using namespace ember;
using Catch::Approx;

namespace {
// Mono clip with the given samples (frameCount == samples.size(), channels == 1).
std::shared_ptr<AudioClip> monoClip(std::vector<float> s) {
    auto c = std::make_shared<AudioClip>();
    c->channels   = 1;
    c->sampleRate = 8;
    c->frameCount = s.size();
    c->samples    = std::move(s);
    return c;
}
} // namespace

TEST_CASE("AudioEngine mixes a one-shot voice then retires it", "[audio]") {
    AudioEngine eng;                       // no device; mixer only
    auto clip = monoClip({1, 2, 3, 4});
    auto h = eng.play(clip);
    REQUIRE(h.valid());
    REQUIRE(eng.activeVoices() == 1);

    std::vector<f32> out(4, -99.0f);
    eng.renderInto(out.data(), 4, 1);
    REQUIRE(out[0] == Approx(1.0f));
    REQUIRE(out[3] == Approx(4.0f));

    eng.renderInto(out.data(), 4, 1);      // past the end -> retires, silence
    REQUIRE(out[0] == Approx(0.0f));
    REQUIRE(eng.activeVoices() == 0);
}

TEST_CASE("AudioEngine looping wraps the cursor", "[audio]") {
    AudioEngine eng;
    auto h = eng.play(monoClip({1, 2, 3, 4}), PlayParams{1.0f, 1.0f, /*loop*/true, AudioBus::SFX});
    std::vector<f32> out(8, 0.0f);
    eng.renderInto(out.data(), 8, 1);
    REQUIRE(out[4] == Approx(1.0f));       // wrapped back to frame 0
    REQUIRE(out[7] == Approx(4.0f));
    REQUIRE(eng.isPlaying(h));
}

TEST_CASE("AudioEngine applies master and bus gain", "[audio]") {
    AudioEngine eng;
    eng.setMasterVolume(0.5f);
    eng.setBusVolume(AudioBus::SFX, 0.5f);
    eng.play(monoClip({1, 1, 1, 1}), PlayParams{1.0f, 1.0f, false, AudioBus::SFX});
    std::vector<f32> out(4, 0.0f);
    eng.renderInto(out.data(), 4, 1);
    REQUIRE(out[0] == Approx(0.25f));      // 1 * voice(1) * bus(0.5) * master(0.5)
}

TEST_CASE("AudioEngine sums multiple voices and stop() silences one", "[audio]") {
    AudioEngine eng;
    // 8-frame clips so a voice still has samples left after the first 4-frame render.
    eng.play(monoClip({1, 1, 1, 1, 1, 1, 1, 1}));
    auto h2 = eng.play(monoClip({2, 2, 2, 2, 2, 2, 2, 2}));
    std::vector<f32> out(4, 0.0f);
    eng.renderInto(out.data(), 4, 1);
    REQUIRE(out[0] == Approx(3.0f));       // 1 + 2
    REQUIRE(eng.activeVoices() == 2);

    eng.stop(h2);
    eng.renderInto(out.data(), 4, 1);      // voice 1 plays frames 4..7 (still 1s)
    REQUIRE(out[0] == Approx(1.0f));
    REQUIRE(eng.activeVoices() == 1);
}
