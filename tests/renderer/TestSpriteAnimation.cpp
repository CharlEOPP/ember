#include <catch2/catch_test_macros.hpp>
#include "ember/renderer/SpriteAnimationSystem.h"
#include "ember/renderer/SpriteAnimation.h"
#include "ember/renderer/Components2D.h"
#include "ember/assets/AssetManager.h"
#include "ember/ecs/World.h"

#include <memory>

using namespace ember;

namespace {
// Returns a fixed 4-frame clip regardless of path.
struct FixedClipLoader : IAssetLoader<SpriteAnimationClip> {
    bool loop;
    explicit FixedClipLoader(bool l) : loop(l) {}
    std::shared_ptr<SpriteAnimationClip> load(const std::filesystem::path&, const AssetSettings&) override {
        auto c = std::make_shared<SpriteAnimationClip>();
        c->fps = 4.0f; c->loop = loop;
        c->frames = { {0.0f,0,0.25f,1}, {0.25f,0,0.5f,1}, {0.5f,0,0.75f,1}, {0.75f,0,1.0f,1} };
        return c;
    }
};

Entity makeAnimated(World& w, AssetManager& mgr) {
    Entity e = w.create();
    w.emplace<SpriteRenderer>(e);
    auto& a = w.emplace<SpriteAnimator>(e);
    a.clip = mgr.load<SpriteAnimationClip>("anim");
    return e;
}
} // namespace

TEST_CASE("SpriteAnimator advances frames at fps", "[renderer]") {
    AssetManager mgr;
    mgr.registerLoader<SpriteAnimationClip>(std::make_shared<FixedClipLoader>(true));
    World w;
    Entity e = makeAnimated(w, mgr);
    SpriteAnimationSystem sys(mgr);

    sys.update(w, 0.0f);                                    // t=0 -> frame 0
    REQUIRE(w.get<SpriteRenderer>(e).uvRect.x == 0.0f);
    sys.update(w, 0.25f);                                   // t=0.25, 4fps -> frame 1
    REQUIRE(w.get<SpriteRenderer>(e).uvRect.x == 0.25f);
    sys.update(w, 0.25f);                                   // frame 2
    REQUIRE(w.get<SpriteRenderer>(e).uvRect.x == 0.5f);
}

TEST_CASE("Looping animation wraps to the first frame", "[renderer]") {
    AssetManager mgr;
    mgr.registerLoader<SpriteAnimationClip>(std::make_shared<FixedClipLoader>(true));
    World w;
    Entity e = makeAnimated(w, mgr);
    SpriteAnimationSystem sys(mgr);
    sys.update(w, 1.0f);                                    // duration = 4/4 = 1s -> wraps to 0
    REQUIRE(w.get<SpriteRenderer>(e).uvRect.x == 0.0f);
}

TEST_CASE("Non-looping animation clamps and stops on the last frame", "[renderer]") {
    AssetManager mgr;
    mgr.registerLoader<SpriteAnimationClip>(std::make_shared<FixedClipLoader>(false));
    World w;
    Entity e = makeAnimated(w, mgr);
    SpriteAnimationSystem sys(mgr);
    sys.update(w, 10.0f);                                   // way past the end
    REQUIRE(w.get<SpriteRenderer>(e).uvRect.x == 0.75f);   // last frame
    REQUIRE_FALSE(w.get<SpriteAnimator>(e).playing);
}
