#pragma once
#include "ember/core/Types.h"
#include <chrono>

namespace ember {

/** High-resolution scope timer. */
class Timer {
public:
    Timer() : m_start(s_clock::now()) {}

    void reset() noexcept { m_start = s_clock::now(); }

    [[nodiscard]] f64 elapsed() const noexcept {
        return std::chrono::duration<f64>(s_clock::now() - m_start).count();
    }

private:
    using s_clock = std::chrono::high_resolution_clock;
    std::chrono::time_point<s_clock> m_start;
};

// ---- Global frame timing — updated once per frame by the main loop ----
namespace Time {
    void update(f32 dt) noexcept;
    [[nodiscard]] f32 deltaTime() noexcept;
    [[nodiscard]] f64 elapsed()   noexcept;
} // namespace Time

} // namespace ember
