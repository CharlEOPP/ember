#include "ember/renderer/SpriteRenderSystem.h"
#include "ember/renderer/Renderer2D.h"
#include "ember/renderer/Components2D.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"
#include "ember/assets/AssetManager.h"
#include "ember/assets/Texture2D.h"
#include "ember/assets/loaders/Texture2DLoader.h"

#include <algorithm>
#include <type_traits>
#include <vector>

namespace ember {

void SpriteRenderSystem::update(World& world, f32 /*dt*/) {
    struct Item { Entity entity; const WorldTransform* wt; const SpriteRenderer* sprite; };
    std::vector<Item> items;

    for (auto [e, wt, sprite] : world.view<WorldTransform, SpriteRenderer>())
        items.push_back({ e, &wt, &sprite });

    // Stable sort by layer; equal layers keep submission (view) order.
    std::stable_sort(items.begin(), items.end(),
                     [](const Item& a, const Item& b) { return a.sprite->layer < b.sprite->layer; });

    for (const Item& it : items) {
        const auto id = static_cast<f32>(static_cast<std::underlying_type_t<Entity>>(it.entity));

        // Resolve the sprite's texture handle. A valid, resident handle yields
        // its RHI texture; a set-but-not-yet-resident handle (async) or a bad
        // path shows the missing-texture placeholder; an empty handle with no
        // path renders as a flat-color quad (null texture).
        std::shared_ptr<ITexture2D> tex;
        if (it.sprite->texture.valid()) {
            if (Texture2D* t = m_assets ? m_assets->get(it.sprite->texture) : nullptr)
                tex = t->texture;
            else
                tex = Texture2DLoader::missingTexture();
        }
        Renderer2D::drawSprite(it.wt->matrix, *it.sprite, tex, id);
    }
}

} // namespace ember
