#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Reflect.h"

#include <glm/glm.hpp>
#include <vector>

namespace ember {

struct Particle {
    glm::vec2 position{0.0f};
    glm::vec2 velocity{0.0f};
    f32 age      = 0.0f;
    f32 lifetime = 1.0f;
};

// Emits and simulates 2D particles. Simulation (`update`/`burst`) is inline and
// GL-free so it's headless-testable; ParticleSystem renders the particles.
struct ParticleEmitter {
    // ---- config (reflected) ----
    f32       emitRate   = 20.0f;          // particles/sec (continuous)
    f32       lifeMin    = 0.5f;
    f32       lifeMax    = 1.5f;
    f32       startSize  = 0.2f;
    f32       endSize    = 0.0f;
    glm::vec4 startColor {1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 endColor   {1.0f, 1.0f, 1.0f, 0.0f};
    glm::vec2 velMin     {-1.0f, 1.0f};
    glm::vec2 velMax     { 1.0f, 3.0f};
    glm::vec2 gravity    { 0.0f, -2.0f};
    u32       maxParticles = 512;
    bool      playing    = true;

    // ---- runtime (not reflected) ----
    std::vector<Particle> particles;
    f32       emitAccum  = 0.0f;
    u32       rngState   = 0x12345u;
    glm::vec2 origin     {0.0f};   // emitter world position, refreshed by the system

    [[nodiscard]] usize liveCount() const { return particles.size(); }

    // Advance one frame: continuous emission (if playing) + integrate + cull.
    void update(const glm::vec2& worldOrigin, f32 dt) {
        origin = worldOrigin;
        if (playing) {
            emitAccum += emitRate * dt;
            while (emitAccum >= 1.0f) { emitAccum -= 1.0f; spawn(); }
        }
        for (Particle& p : particles) {
            p.velocity += gravity * dt;
            p.position += p.velocity * dt;
            p.age      += dt;
        }
        usize w = 0;
        for (usize r = 0; r < particles.size(); ++r)
            if (particles[r].age < particles[r].lifetime) particles[w++] = particles[r];
        particles.resize(w);
    }

    // Emit `n` particles immediately (e.g. a pickup puff).
    void burst(u32 n) { for (u32 i = 0; i < n; ++i) spawn(); }

private:
    f32 rand01() {
        rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;
        return static_cast<f32>(rngState & 0xFFFFFFu) / static_cast<f32>(0xFFFFFF);
    }
    f32 lerp(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
    void spawn() {
        if (particles.size() >= maxParticles) return;
        Particle p;
        p.position = origin;
        p.lifetime = lerp(lifeMin, lifeMax, rand01());
        p.age      = 0.0f;
        p.velocity = { lerp(velMin.x, velMax.x, rand01()), lerp(velMin.y, velMax.y, rand01()) };
        particles.push_back(p);
    }
};

EMBER_REFLECT(ParticleEmitter) {
    EMBER_FIELD(emitRate);
    EMBER_FIELD(lifeMin);  EMBER_FIELD(lifeMax);
    EMBER_FIELD(startSize); EMBER_FIELD(endSize);
    EMBER_FIELD(startColor); EMBER_FIELD(endColor);
    EMBER_FIELD(velMin); EMBER_FIELD(velMax);
    EMBER_FIELD(gravity);
    EMBER_FIELD(maxParticles);
    EMBER_FIELD(playing);
}

} // namespace ember
