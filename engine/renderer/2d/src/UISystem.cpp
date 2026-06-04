#include "ember/renderer/UISystem.h"
#include "ember/renderer/UIComponents.h"
#include "ember/renderer/Renderer2D.h"
#include "ember/assets/AssetManager.h"
#include "ember/assets/Texture2D.h"
#include "ember/assets/FontAsset.h"
#include "ember/input/Input.h"
#include "ember/ecs/World.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <vector>

namespace ember {

namespace {
// Center + extent for Renderer2D::drawQuad (unit quad spans [-0.5, 0.5]).
inline glm::vec2 rectCenter(const glm::vec4& r) { return {r.x + r.z * 0.5f, r.y + r.w * 0.5f}; }
inline glm::vec2 rectSize  (const glm::vec4& r) { return {r.z, r.w}; }
} // namespace

void UISystem::update(World& world, f32 /*dt*/) {
    // ---- 1. Pointer -> button state ----
    const glm::vec2 mouse    = Input::getMousePosition();
    const bool      down     = Input::isMouseButtonDown(Mouse::Left);
    const bool      released = Input::isMouseButtonReleased(Mouse::Left);
    for (auto [e, btn] : world.view<UIButton>())
        updateButtonState(btn, m_screen, mouse, down, released);

    // ---- 2. Draw, ordered by `order` (low first) ----
    enum class Kind { Image, Button, Text };
    struct Item { Entity e; Kind kind; i32 order; };
    std::vector<Item> items;
    for (auto [e, img] : world.view<UIImage>())  if (img.visible) items.push_back({e, Kind::Image,  img.order});
    for (auto [e, btn] : world.view<UIButton>()) if (btn.visible) items.push_back({e, Kind::Button, btn.order});
    for (auto [e, txt] : world.view<UIText>())   if (txt.visible) items.push_back({e, Kind::Text,   txt.order});
    std::stable_sort(items.begin(), items.end(),
                     [](const Item& a, const Item& b) { return a.order < b.order; });

    // Screen-space ortho: top-left origin, +y down.
    const glm::mat4 proj = glm::ortho(0.0f, m_screen.x, m_screen.y, 0.0f, -1.0f, 1.0f);
    Renderer2D::beginScene(proj);

    for (const Item& it : items) {
        switch (it.kind) {
        case Kind::Image: {
            auto& img = world.get<UIImage>(it.e);
            if (!img.texture.valid() && !img.texturePath.empty() && m_assets)
                img.texture = m_assets->load<Texture2D>(img.texturePath);
            const glm::vec4 r = computeScreenRect(img.rect, m_screen);
            if (Texture2D* t = m_assets ? m_assets->get<Texture2D>(img.texture) : nullptr; t && t->texture)
                Renderer2D::drawQuad(rectCenter(r), rectSize(r), t->texture, img.color);
            else
                Renderer2D::drawQuad(rectCenter(r), rectSize(r), img.color);
            break;
        }
        case Kind::Button: {
            auto& btn = world.get<UIButton>(it.e);
            const glm::vec4 r = computeScreenRect(btn.rect, m_screen);
            Renderer2D::drawQuad(rectCenter(r), rectSize(r), btn.currentColor());
            break;
        }
        case Kind::Text: {
            auto& txt = world.get<UIText>(it.e);
            if (txt.text.empty()) break;
            if (!txt.font.valid() && !txt.fontPath.empty() && m_assets)
                txt.font = m_assets->load<FontAsset>(txt.fontPath);
            if (FontAsset* f = m_assets ? m_assets->get<FontAsset>(txt.font) : nullptr) {
                const glm::vec4 r = computeScreenRect(txt.rect, m_screen);
                Renderer2D::drawText(txt.text, *f, glm::vec2(r.x, r.y), txt.fontScale, txt.color);
            }
            break;
        }
        }
    }

    Renderer2D::endScene();
}

} // namespace ember
