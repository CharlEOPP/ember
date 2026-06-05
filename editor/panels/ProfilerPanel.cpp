#include "ProfilerPanel.h"

#include "ember/core/Profiler.h"
#include "ember/renderer/Renderer2D.h"

#include <imgui.h>
#include <algorithm>
#include <cstdint>

namespace ember {

namespace {
ImU32 colorForName(const std::string& s) {
    std::uint32_t h = 2166136261u;                 // FNV-1a
    for (char c : s) { h ^= static_cast<std::uint8_t>(c); h *= 16777619u; }
    return IM_COL32(120 + (h & 0x7F), 120 + ((h >> 8) & 0x7F), 120 + ((h >> 16) & 0x7F), 255);
}
} // namespace

void ProfilerPanel::onImGuiRender(EditorContext& /*ctx*/, f32 dt) {
    m_frameMs[m_head] = dt * 1000.0f;
    m_head = (m_head + 1) % kHistory;

    ImGui::Begin("Profiler");

    // ---- Stats bar ----
    const float frameMs = dt * 1000.0f;
    const float fps     = dt > 0.0f ? 1.0f / dt : 0.0f;
    const RendererStats& s = Renderer2D::stats();
    ImGui::Text("%.2f ms  |  %.0f FPS  |  draws %u  quads %u  verts %u",
                frameMs, fps, s.drawCalls, s.quadCount, s.vertexCount);

    Profiler& prof = Profiler::instance();
    bool enabled = prof.enabled();
    if (ImGui::Checkbox("Record", &enabled)) prof.setEnabled(enabled);

    // ---- Rolling frame-time graph (120 frames) ----
    float maxMs = 1.0f;
    for (float v : m_frameMs) maxMs = std::max(maxMs, v);
    ImGui::PlotLines("##frametime", m_frameMs, kHistory, m_head, "frame ms",
                     0.0f, maxMs * 1.2f, ImVec2(-1.0f, 60.0f));

    ImGui::Separator();

    // ---- Flame graph of the last frame ----
    const auto& samples = prof.lastFrame();
    if (samples.empty()) {
        ImGui::TextDisabled("No scope samples. Build with -DEMBER_ENABLE_PROFILING=ON.");
        ImGui::End();
        return;
    }

    f64 t0 = samples.front().start, t1 = samples.front().end;
    for (const ProfileSample& sm : samples) { t0 = std::min(t0, sm.start); t1 = std::max(t1, sm.end); }
    const f64 dur = std::max(1e-6, t1 - t0);

    const ImVec2  origin = ImGui::GetCursorScreenPos();
    const float   width  = ImGui::GetContentRegionAvail().x;
    const float   rowH   = 18.0f;
    ImDrawList*   dl     = ImGui::GetWindowDrawList();
    const ImVec2  mouse  = ImGui::GetMousePos();
    int maxDepth = 0;

    for (const ProfileSample& sm : samples) {
        const float x = origin.x + static_cast<float>((sm.start - t0) / dur) * width;
        const float w = std::max(1.0f, static_cast<float>((sm.end - sm.start) / dur) * width);
        const float y = origin.y + static_cast<float>(sm.depth) * rowH;
        maxDepth = std::max(maxDepth, sm.depth);

        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + rowH - 2.0f), colorForName(sm.name));
        dl->PushClipRect(ImVec2(x, y), ImVec2(x + w, y + rowH), true);
        dl->AddText(ImVec2(x + 3.0f, y + 2.0f), IM_COL32(15, 15, 15, 255), sm.name.c_str());
        dl->PopClipRect();

        if (mouse.x >= x && mouse.x <= x + w && mouse.y >= y && mouse.y <= y + rowH)
            ImGui::SetTooltip("%s: %.3f ms", sm.name.c_str(), sm.ms());
    }

    ImGui::Dummy(ImVec2(width, static_cast<float>(maxDepth + 1) * rowH));
    ImGui::End();
}

} // namespace ember
