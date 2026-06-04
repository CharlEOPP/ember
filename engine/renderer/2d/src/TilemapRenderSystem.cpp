#include "ember/renderer/TilemapRenderSystem.h"
#include "ember/renderer/Tilemap.h"
#include "ember/renderer/Renderer2D.h"
#include "ember/assets/AssetManager.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"   // Transform

#include <glm/gtc/matrix_transform.hpp>

namespace ember {

void TilemapRenderSystem::update(World& world, f32 /*dt*/) {
    for (auto [e, t, map] : world.view<Transform, Tilemap>()) {
        if (map.width == 0 || map.height == 0) continue;

        // Lazily resolve the tileset texture.
        if (!map.tileset.texture.valid() && !map.tileset.texturePath.empty() && m_assets)
            map.tileset.texture = m_assets->load<Texture2D>(map.tileset.texturePath);
        Texture2D* tex = m_assets ? m_assets->get<Texture2D>(map.tileset.texture) : nullptr;
        const auto rhiTex = tex ? tex->texture : nullptr;

        const f32 s = map.tileWorldSize;
        const glm::vec2 origin{t.position.x, t.position.y};
        for (u32 y = 0; y < map.height; ++y) {
            for (u32 x = 0; x < map.width; ++x) {
                const u32 id = map.at(x, y);
                if (id == 0) continue;
                const glm::vec2 pos = origin + glm::vec2(static_cast<f32>(x) * s, static_cast<f32>(y) * s);
                glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(pos, 0.0f))
                            * glm::scale(glm::mat4(1.0f), glm::vec3(s, s, 1.0f));
                Renderer2D::drawQuad(m, glm::vec4(1.0f), rhiTex, map.tileset.uvForTile(id));
            }
        }
    }
}

} // namespace ember
