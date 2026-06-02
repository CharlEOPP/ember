#pragma once
#include "ember/core/Log.h"
#include <functional>

// ---- Cross-platform debug break ----
#if defined(_MSC_VER)
#  define EMBER_DEBUGBREAK() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__)
#  define EMBER_DEBUGBREAK() __builtin_trap()
#else
#  include <csignal>
#  define EMBER_DEBUGBREAK() std::raise(SIGTRAP)
#endif

namespace ember::Assert {

using Handler = std::function<void(const char* expr, const char* file, int line, const char* msg)>;

void setHandler(Handler handler);
void resetHandler();
void invoke(const char* expr, const char* file, int line, const char* msg);

} // namespace ember::Assert

// ---- EMBER_ASSERT -- debug only, no-op in Release ----
#if !defined(NDEBUG)
#  define EMBER_ASSERT(cond, fmt, ...)                                                \
     do {                                                                              \
         if (!(cond)) {                                                                \
             EMBER_LOG_ERROR("Assertion failed: ({}) -- " fmt, #cond, ##__VA_ARGS__);\
             ::ember::Assert::invoke(#cond, __FILE__, __LINE__, fmt);                 \
         }                                                                             \
     } while(0)
#else
#  define EMBER_ASSERT(cond, ...) do {} while(0)
#endif

// ---- EMBER_VERIFY -- active in all builds ----
#define EMBER_VERIFY(cond, fmt, ...)                                                  \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            EMBER_LOG_ERROR("Verify failed: ({}) -- " fmt, #cond, ##__VA_ARGS__);    \
            ::ember::Assert::invoke(#cond, __FILE__, __LINE__, fmt);                  \
        }                                                                              \
    } while(0)
