#pragma once
#include "ember/core/Types.h"

#include <string>
#include <vector>

namespace ember {

// One timed scope within a frame. `depth` is the nesting level (0 = top-level).
// Times are seconds from a steady clock; `end < start` only while still open.
struct ProfileSample {
    std::string name;
    int depth = 0;
    f64 start = 0.0;
    f64 end   = 0.0;
    [[nodiscard]] f64 ms() const { return (end - start) * 1000.0; }
};

// Per-frame scope collector. ImGui/GL-free; lives in ember_core so any module can
// be instrumented. Collection happens only via EMBER_PROFILE_SCOPE, which expands
// to nothing unless EMBER_ENABLE_PROFILING is defined — so it is zero-cost by
// default. The Profiler object itself always exists (the editor panel reads it).
class Profiler {
public:
    static Profiler& instance();

    void beginFrame();              // start collecting (clears the current buffer)
    void endFrame();                // publish: current → lastFrame

    int  beginScope(const char* name);   // returns sample index (or -1 if disabled)
    void endScope(int index);

    [[nodiscard]] const std::vector<ProfileSample>& lastFrame() const { return m_last; }
    [[nodiscard]] bool   enabled() const { return m_enabled; }
    void setEnabled(bool e) { m_enabled = e; }

private:
    std::vector<ProfileSample> m_current;
    std::vector<ProfileSample> m_last;
    int  m_depth   = 0;
    bool m_enabled = true;
};

// RAII scope timer (used by the EMBER_PROFILE_SCOPE macro).
class ProfileScope {
public:
    explicit ProfileScope(const char* name) : m_index(Profiler::instance().beginScope(name)) {}
    ~ProfileScope() { Profiler::instance().endScope(m_index); }
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;
private:
    int m_index;
};

} // namespace ember

#define EMBER_PROF_CONCAT2(a, b) a##b
#define EMBER_PROF_CONCAT(a, b)  EMBER_PROF_CONCAT2(a, b)

#ifdef EMBER_ENABLE_PROFILING
    #define EMBER_PROFILE_SCOPE(name) \
        ::ember::ProfileScope EMBER_PROF_CONCAT(_emberProfScope_, __LINE__){name}
#else
    #define EMBER_PROFILE_SCOPE(name) ((void)0)
#endif
