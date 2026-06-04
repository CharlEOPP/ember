#include "ember/renderer/ParticleSystem.h"
#include "ember/renderer/ParticleEmitter.h"
#include "ember/renderer/Renderer2D.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"   // Transform

#include <glm/gtc/matrix_transform.hpp>

namespace ember {

void ParticleSystem::update(World& world, f32 dt) {
    for (auto [e, t, emitter] : world.view<Transform, ParticleEmitter>()) {
        emitter.update(glm::vec2(t.position.x, t.position.y), dt);
        for (const Particle& p : emitter.particles) {
            const f32 u = p.lifetime > 0.0f ? p.age / p.lifetime : 1.0f;   // 0..1 over life
            const f32 size = emitter.startSize + (emitter.endSize - emitter.startSize) * u;
            const glm::vec4 color = emitter.startColor + (emitter.endColor - emitter.startColor) * u;
            glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(p.position, 0.0f))
                        * glm::scale(glm::mat4(1.0f), glm::vec3(size, size, 1.0f));
            Renderer2D::drawQuad(m, color, nullptr, 1.0f, -1.0f);
        }
    }
}

} // namespace ember
