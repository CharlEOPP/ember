#include "ember/renderer/Camera.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"
#include "ember/core/Log.h"

#include <glm/gtc/matrix_transform.hpp>

namespace ember {

void CameraSystem::update(World& world, f32 /*dt*/) {
    m_hasCamera = false;
    Entity chosen = NULL_ENTITY;
    int primaryCount = 0;

    for (auto [e, cam, wt] : world.view<Camera2D, WorldTransform>()) {
        (void)wt;
        if (cam.isPrimary) { ++primaryCount; if (chosen == NULL_ENTITY) chosen = e; }
        else if (chosen == NULL_ENTITY) chosen = e;   // fall back to any camera
    }

    if (chosen == NULL_ENTITY) return;
    if (primaryCount > 1)
        EMBER_LOG_WARN("CameraSystem: {} cameras marked isPrimary; using the first", primaryCount);

    const Camera2D&       cam = world.get<Camera2D>(chosen);
    const WorldTransform& wt  = world.get<WorldTransform>(chosen);

    const glm::mat4 view = glm::inverse(wt.matrix);
    const glm::mat4 proj = glm::ortho(-cam.size * m_aspect, cam.size * m_aspect,
                                      -cam.size, cam.size, cam.nearClip, cam.farClip);
    m_viewProjection = proj * view;
    m_hasCamera = true;
}

glm::vec2 worldToScreen(const glm::vec3& world, const glm::mat4& viewProjection, const glm::vec2& viewport) {
    glm::vec4 clip = viewProjection * glm::vec4(world, 1.0f);
    if (clip.w != 0.0f) { clip.x /= clip.w; clip.y /= clip.w; }
    return {
        (clip.x * 0.5f + 0.5f) * viewport.x,
        (1.0f - (clip.y * 0.5f + 0.5f)) * viewport.y
    };
}

glm::vec3 screenToWorld(const glm::vec2& screen, const glm::mat4& viewProjection, const glm::vec2& viewport) {
    const glm::vec4 ndc{
        (screen.x / viewport.x) * 2.0f - 1.0f,
        1.0f - (screen.y / viewport.y) * 2.0f,
        0.0f, 1.0f
    };
    glm::vec4 world = glm::inverse(viewProjection) * ndc;
    if (world.w != 0.0f) world /= world.w;
    return glm::vec3(world);
}

} // namespace ember
