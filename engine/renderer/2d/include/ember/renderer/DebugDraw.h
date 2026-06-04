#pragma once
#include "ember/core/Types.h"
#include <glm/glm.hpp>

// Immediate-mode debug line drawing. In Release builds every call is an inline
// no-op (zero runtime cost). In Debug, lines are batched and flushed at frame end.
namespace ember::DebugDraw {

#if !defined(NDEBUG)
void init();
void shutdown();
void begin(const glm::mat4& viewProjection);
void line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& color);
void rect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
void circle(const glm::vec2& center, f32 radius, const glm::vec4& color, int segments = 32);
void flush();
#else
inline void init() {}
inline void shutdown() {}
inline void begin(const glm::mat4&) {}
inline void line(const glm::vec2&, const glm::vec2&, const glm::vec4&) {}
inline void rect(const glm::vec2&, const glm::vec2&, const glm::vec4&) {}
inline void circle(const glm::vec2&, f32, const glm::vec4&, int = 32) {}
inline void flush() {}
#endif

} // namespace ember::DebugDraw
