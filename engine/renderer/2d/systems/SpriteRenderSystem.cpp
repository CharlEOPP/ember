#include "ember/renderer/SpriteRenderSystem.h"
#include "ember/renderer/Renderer2D.h"
#include "ember/renderer/Components2D.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"

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
        Renderer2D::drawSprite(it.wt->matrix, *it.sprite, id);
    }
}

} // namespace ember
