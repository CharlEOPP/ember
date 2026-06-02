#include "ember/platform/Timer.h"

namespace ember::Time {

static f32 s_deltaTime = 0.0f;
static f64 s_elapsed   = 0.0;

void update(f32 dt) noexcept {
    s_deltaTime  = dt;
    s_elapsed   += static_cast<f64>(dt);
}

f32 deltaTime() noexcept { return s_deltaTime; }
f64 elapsed()   noexcept { return s_elapsed;   }

} // namespace ember::Time
