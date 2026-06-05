#pragma once
#include <glm/glm.hpp>
#include <cmath>

// Pure, ImGui-free gizmo math (snap helpers) — unit-testable in ember_editor_core.
namespace ember::GizmoMath {

inline glm::vec3 snapTranslate(const glm::vec3& v, float grid) {
    if (grid <= 0.0f) return v;
    return { std::round(v.x / grid) * grid,
             std::round(v.y / grid) * grid,
             std::round(v.z / grid) * grid };
}

inline float snapAngleDeg(float deg, float stepDeg) {
    if (stepDeg <= 0.0f) return deg;
    return std::round(deg / stepDeg) * stepDeg;
}

} // namespace ember::GizmoMath
