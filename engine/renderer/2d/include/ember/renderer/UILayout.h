#pragma once
#include "ember/core/Types.h"

#include <glm/glm.hpp>

namespace ember {

// Where an element is pinned within the screen. The element's own matching pivot
// is aligned to that screen point, so e.g. TopRight pins the element's top-right
// corner to the screen's top-right corner.
enum class UIAnchor : u32 {
    TopLeft, Top, TopRight,
    Left,    Center, Right,
    BottomLeft, Bottom, BottomRight
};

// Screen-space rectangle in pixels (origin top-left, +y down).
struct UIRect {
    UIAnchor  anchor = UIAnchor::Center;
    glm::vec2 offset {0.0f, 0.0f};   // pixels from the anchor point
    glm::vec2 size   {100.0f, 40.0f};
};

// Fractional position of an anchor along each axis (0=left/top, 1=right/bottom).
inline glm::vec2 anchorFraction(UIAnchor a) {
    const u32 i = static_cast<u32>(a);
    const f32 fx = static_cast<f32>(i % 3) * 0.5f;   // col 0,1,2 -> 0,0.5,1
    const f32 fy = static_cast<f32>(i / 3) * 0.5f;   // row 0,1,2 -> 0,0.5,1
    return {fx, fy};
}

// Resolve a UIRect to an absolute screen rect {x, y, w, h} (top-left origin).
inline glm::vec4 computeScreenRect(const UIRect& r, const glm::vec2& screen) {
    const glm::vec2 f = anchorFraction(r.anchor);
    const glm::vec2 anchorPt = f * screen;
    const glm::vec2 topLeft  = anchorPt + r.offset - f * r.size;   // align element's matching pivot
    return {topLeft.x, topLeft.y, r.size.x, r.size.y};
}

// Point-in-rect test (rect = {x, y, w, h}).
inline bool hitTest(const glm::vec4& rect, const glm::vec2& p) {
    return p.x >= rect.x && p.x <= rect.x + rect.z &&
           p.y >= rect.y && p.y <= rect.y + rect.w;
}

} // namespace ember
