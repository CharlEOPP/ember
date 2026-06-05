#pragma once
#include "ember/core/Types.h"
#include "../EditorContext.h"

namespace ember {

// Visualizes the engine Profiler: a stats bar, a rolling frame-time graph, and a
// hand-drawn flame graph of the last frame's EMBER_PROFILE_SCOPE samples.
class ProfilerPanel {
public:
    void onImGuiRender(EditorContext& ctx, f32 dt);

private:
    static constexpr int kHistory = 120;
    float m_frameMs[kHistory] = {};
    int   m_head = 0;
};

} // namespace ember
