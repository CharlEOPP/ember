#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Reflect.h"
#include "ember/renderer/UILayout.h"
#include "ember/assets/AssetHandle.h"
#include "ember/assets/Texture2D.h"

#include <glm/glm.hpp>
#include <string>

namespace ember {

class FontAsset;

// A screen-space colored/textured panel. `texture` is resolved from `texturePath`
// by UISystem through the AssetManager (null handle ⇒ flat color).
struct UIImage {
    UIRect      rect;
    glm::vec4   color{1.0f};
    std::string texturePath;
    AssetHandle<Texture2D> texture;
    bool        visible = true;
    i32         order   = 0;   // draw order within the UI pass (low first)
};

// A screen-space text label. Font is resolved from `fontPath` by UISystem.
struct UIText {
    UIRect      rect;
    std::string text;
    f32         fontScale = 1.0f;
    glm::vec4   color{1.0f};
    std::string fontPath;
    AssetHandle<FontAsset> font;
    bool        visible = true;
    i32         order   = 0;
};

// A clickable button. UISystem fills the runtime flags each frame from the
// pointer state; gameplay reads `clicked` (true for one frame on release inside).
struct UIButton {
    UIRect      rect;
    glm::vec4   normalColor {0.25f, 0.25f, 0.25f, 1.0f};
    glm::vec4   hoverColor  {0.35f, 0.35f, 0.35f, 1.0f};
    glm::vec4   pressedColor{0.15f, 0.15f, 0.15f, 1.0f};
    bool        visible = true;
    i32         order   = 0;
    // ---- runtime (not reflected) ----
    bool        hovered = false;
    bool        pressed = false;
    bool        clicked = false;

    [[nodiscard]] glm::vec4 currentColor() const {
        if (pressed) return pressedColor;
        if (hovered) return hoverColor;
        return normalColor;
    }
};

// Update a button's runtime flags from pointer state. Headless-testable (no GL).
//   mouse       : pointer position in screen pixels (top-left origin)
//   mouseDown   : pointer is currently held this frame
//   mouseReleased: pointer was released this frame
inline void updateButtonState(UIButton& b, const glm::vec2& screen,
                              const glm::vec2& mouse, bool mouseDown, bool mouseReleased) {
    const glm::vec4 r = computeScreenRect(b.rect, screen);
    const bool inside = hitTest(r, mouse);
    b.hovered = inside;
    b.clicked = false;
    if (inside && mouseDown) b.pressed = true;
    if (mouseReleased) {
        if (b.pressed && inside) b.clicked = true;
        b.pressed = false;
    }
}

EMBER_REFLECT(UIImage) {
    EMBER_FIELD(texturePath);
    EMBER_FIELD(color);
    EMBER_FIELD(visible);
    EMBER_FIELD(order);
}

EMBER_REFLECT(UIText) {
    EMBER_FIELD(text);
    EMBER_FIELD(fontScale);
    EMBER_FIELD(color);
    EMBER_FIELD(fontPath);
    EMBER_FIELD(visible);
    EMBER_FIELD(order);
}

EMBER_REFLECT(UIButton) {
    EMBER_FIELD(normalColor);
    EMBER_FIELD(hoverColor);
    EMBER_FIELD(pressedColor);
    EMBER_FIELD(visible);
    EMBER_FIELD(order);
}

} // namespace ember
