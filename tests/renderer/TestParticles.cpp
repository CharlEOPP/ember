#include <catch2/catch_test_macros.hpp>
#include "ember/renderer/ParticleEmitter.h"

using namespace ember;

TEST_CASE("ParticleEmitter emits at the configured rate", "[particles]") {
    ParticleEmitter e;
    e.emitRate = 10.0f;
    e.lifeMin = e.lifeMax = 100.0f;   // long-lived so none die during the window
    e.gravity = {0.0f, 0.0f};

    e.update({0.0f, 0.0f}, 1.0f);     // 10 particles/sec * 1s = 10
    REQUIRE(e.liveCount() == 10);

    e.update({0.0f, 0.0f}, 0.5f);     // +5
    REQUIRE(e.liveCount() == 15);
}

TEST_CASE("Particles are culled when their lifetime expires", "[particles]") {
    ParticleEmitter e;
    e.playing = false;                // emit manually so the count is deterministic
    e.lifeMin = e.lifeMax = 1.0f;
    e.gravity = {0.0f, 0.0f};
    e.burst(5);
    REQUIRE(e.liveCount() == 5);

    e.update({0.0f, 0.0f}, 0.5f);     // age 0.5 < 1.0 -> all alive
    REQUIRE(e.liveCount() == 5);

    e.update({0.0f, 0.0f}, 0.6f);     // age 1.1 >= 1.0 -> all culled
    REQUIRE(e.liveCount() == 0);
}

TEST_CASE("Emission is capped at maxParticles", "[particles]") {
    ParticleEmitter e;
    e.maxParticles = 8;
    e.lifeMin = e.lifeMax = 100.0f;
    e.emitRate = 1000.0f;
    e.update({0.0f, 0.0f}, 1.0f);     // would emit 1000, capped at 8
    REQUIRE(e.liveCount() == 8);

    e.burst(100);                     // burst also respects the cap
    REQUIRE(e.liveCount() == 8);
}

TEST_CASE("Gravity and velocity integrate the particle position", "[particles]") {
    ParticleEmitter e;
    e.playing = false;
    e.lifeMin = e.lifeMax = 100.0f;
    e.velMin = e.velMax = glm::vec2(2.0f, 0.0f);   // deterministic velocity
    e.gravity = {0.0f, -10.0f};
    e.burst(1);

    e.update({0.0f, 0.0f}, 1.0f);
    const Particle& p = e.particles.front();
    // v += g*dt => (2, -10); pos += v*dt => (2, -10)
    REQUIRE(p.velocity.y == -10.0f);
    REQUIRE(p.position.x == 2.0f);
    REQUIRE(p.position.y == -10.0f);
}
