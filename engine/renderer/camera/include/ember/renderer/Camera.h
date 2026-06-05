#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/SystemScheduler.h"
#include "ember/renderer/Components2D.h"

#include <glm/glm.hpp>

namespace ember {

// Computes the primary Camera2D's view-projection each Update phase.
class CameraSystem : public ISystem {
public:
    void update(World& world, f32 dt) override;

    void setViewportSize(u32 width, u32 height) {
        m_aspect = (height == 0) ? 1.0f : static_cast<f32>(width) / static_cast<f32>(height);
    }
    const glm::mat4& viewProjection() const { return m_viewProjection; }
    bool hasCamera() const { return m_hasCamera; }

private:
    glm::mat4 m_viewProjection{1.0f};
    f32       m_aspect = 16.0f / 9.0f;
    bool      m_hasCamera = false;
};

// Free pan/zoom orthographic camera for the editor viewport (not an ECS entity).
class EditorCamera {
public:
    void setViewportSize(u32 width, u32 height);
    void pan(const glm::vec2& delta);
    void zoom(f32 amount);
    const glm::mat4& viewProjection() const { return m_viewProjection; }
    // Separate view / projection (e.g. for ImGuizmo, which decomposes them).
    const glm::mat4& view() const { return m_view; }
    const glm::mat4& projection() const { return m_projection; }
    [[nodiscard]] f32 halfHeight() const { return m_halfHeight; }

private:
    void recompute();
    glm::vec2 m_position{0.0f};
    f32       m_halfHeight = 5.0f;
    f32       m_aspect     = 16.0f / 9.0f;
    glm::mat4 m_view{1.0f};
    glm::mat4 m_projection{1.0f};
    glm::mat4 m_viewProjection{1.0f};
};

// World <-> screen helpers (screen origin top-left, y-down).
glm::vec2 worldToScreen(const glm::vec3& world, const glm::mat4& viewProjection, const glm::vec2& viewport);
glm::vec3 screenToWorld(const glm::vec2& screen, const glm::mat4& viewProjection, const glm::vec2& viewport);

} // namespace ember
