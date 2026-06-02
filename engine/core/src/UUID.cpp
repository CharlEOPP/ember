#include "ember/core/UUID.h"
#include <random>
#include <atomic>
#include <array>
#include <bit>

namespace ember {
namespace {

// xoshiro256++ — Blackman & Vigna 2018
struct Xoshiro256pp {
    std::array<u64, 4> s{};

    u64 next() noexcept {
        const u64 result = std::rotl(s[0] + s[3], 23) + s[0];
        const u64 t = s[1] << 17;
        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = std::rotl(s[3], 45);
        return result;
    }
};

Xoshiro256pp makeSeededRng() {
    std::random_device rd;
    std::uniform_int_distribution<u64> dist;
    Xoshiro256pp rng;
    // Ensure non-zero seed state
    do {
        for (auto& v : rng.s) v = dist(rd);
    } while (rng.s == std::array<u64, 4>{0, 0, 0, 0});
    return rng;
}

static Xoshiro256pp s_rng   = makeSeededRng();
static std::atomic_flag s_lock = ATOMIC_FLAG_INIT;

} // anonymous namespace

UUID generateUUID() {
    UUID result = 0;
    while (result == 0) {
        // Spinlock — UUID generation is infrequent; no need for a mutex
        while (s_lock.test_and_set(std::memory_order_acquire)) {}
        result = s_rng.next();
        s_lock.clear(std::memory_order_release);
    }
    return result;
}

} // namespace ember
