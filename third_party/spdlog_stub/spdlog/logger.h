#pragma once
#include <string>
#include <memory>
#include <vector>
#include <cstdio>

namespace spdlog {

struct sink { virtual ~sink() = default; };
using sink_ptr = std::shared_ptr<sink>;

namespace level {
enum level_enum : int {
    trace=0, debug=1, info=2, warn=3, err=4, critical=5, off=6
};
} // namespace level

class logger {
public:
    template<typename It>
    logger(const std::string& name, It, It) : m_name(name) {}
    explicit logger(const std::string& name) : m_name(name) {}

    const std::string& name() const { return m_name; }

    // Accept any args -- stub does not format, just prints first string arg
    template<typename Fmt, typename... Args>
    void trace   (const Fmt&, Args&&...) {}
    template<typename Fmt, typename... Args>
    void debug   (const Fmt&, Args&&...) {}
    template<typename Fmt, typename... Args>
    void info    (const Fmt& fmt, Args&&...) {}
    template<typename Fmt, typename... Args>
    void warn    (const Fmt&, Args&&...) {}
    template<typename Fmt, typename... Args>
    void error   (const Fmt&, Args&&...) {}
    template<typename Fmt, typename... Args>
    void critical(const Fmt&, Args&&...) {}

    void set_level(level::level_enum) {}
    void set_pattern(const std::string&) {}
    void flush_on(level::level_enum) {}
    void flush() {}

private:
    std::string m_name;
};

} // namespace spdlog
