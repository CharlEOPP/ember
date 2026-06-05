#include "ember/renderer/Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace ember {

void EditorCamera::setViewportSize(u32 width, u32 height) {
    m_aspect = (height == 0) ? 1.0f : static_cast<f32>(width) / static_cast<f32>(height);
    recompute();
}

void EditorCamera::pan(const glm::vec2& delta) {
    m_position += delta;
    recompute();
}

void EditorCamera::zoom(f32 amount) {
    m_halfHeight -= amount;
    if (m_halfHeight < 0.1f) m_halfHeight = 0.1f;
    recompute();
}

void EditorCamera::recompute() {
    m_view = glm::inverse(glm::translate(glm::mat4(1.0f), glm::vec3(m_position, 0.0f)));
    m_projection = glm::ortho(-m_halfHeight * m_aspect, m_halfHeight * m_aspect,
                              -m_halfHeight, m_halfHeight, -1.0f, 1.0f);
    m_viewProjection = m_projection * m_view;
}

} // namespace ember
