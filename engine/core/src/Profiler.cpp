#include "ember/core/Profiler.h"

#include <chrono>

namespace ember {

namespace {
f64 nowSeconds() {
    using clock = std::chrono::steady_clock;
    static const clock::time_point origin = clock::now();
    return std::chrono::duration<f64>(clock::now() - origin).count();
}
} // namespace

Profiler& Profiler::instance() {
    static Profiler p;
    return p;
}

void Profiler::beginFrame() {
    m_current.clear();
    m_depth = 0;
}

void Profiler::endFrame() {
    m_last.swap(m_current);   // publish this frame; m_current reused next beginFrame
}

int Profiler::beginScope(const char* name) {
    if (!m_enabled) return -1;
    const int index = static_cast<int>(m_current.size());
    ProfileSample s;
    s.name  = name;
    s.depth = m_depth;
    s.start = nowSeconds();
    s.end   = s.start;
    m_current.push_back(std::move(s));
    ++m_depth;
    return index;
}

void Profiler::endScope(int index) {
    if (index < 0 || index >= static_cast<int>(m_current.size())) return;
    m_current[static_cast<std::size_t>(index)].end = nowSeconds();
    if (m_depth > 0) --m_depth;
}

} // namespace ember
