#include <catch2/catch_test_macros.hpp>
#include "ember/renderer/UIComponents.h"

using namespace ember;

TEST_CASE("computeScreenRect anchors elements correctly", "[ui]") {
    const glm::vec2 screen{800.0f, 600.0f};

    SECTION("Center pins the element's center to screen center") {
        UIRect r; r.anchor = UIAnchor::Center; r.size = {100.0f, 40.0f};
        const glm::vec4 s = computeScreenRect(r, screen);
        REQUIRE(s.x == 350.0f);   // 400 - 50
        REQUIRE(s.y == 280.0f);   // 300 - 20
    }
    SECTION("TopLeft + offset") {
        UIRect r; r.anchor = UIAnchor::TopLeft; r.size = {100.0f, 40.0f}; r.offset = {10.0f, 10.0f};
        const glm::vec4 s = computeScreenRect(r, screen);
        REQUIRE(s.x == 10.0f);
        REQUIRE(s.y == 10.0f);
    }
    SECTION("BottomRight pins the element's bottom-right corner") {
        UIRect r; r.anchor = UIAnchor::BottomRight; r.size = {100.0f, 40.0f};
        const glm::vec4 s = computeScreenRect(r, screen);
        REQUIRE(s.x == 700.0f);   // 800 - 100
        REQUIRE(s.y == 560.0f);   // 600 - 40
    }
}

TEST_CASE("hitTest detects points inside the rect", "[ui]") {
    const glm::vec4 r{100.0f, 100.0f, 50.0f, 20.0f};
    REQUIRE(hitTest(r, {110.0f, 110.0f}));
    REQUIRE(hitTest(r, {100.0f, 100.0f}));   // top-left edge
    REQUIRE(hitTest(r, {150.0f, 120.0f}));   // bottom-right edge
    REQUIRE_FALSE(hitTest(r, {99.0f, 110.0f}));
    REQUIRE_FALSE(hitTest(r, {110.0f, 130.0f}));
}

TEST_CASE("UIButton click requires press then release inside", "[ui]") {
    const glm::vec2 screen{800.0f, 600.0f};
    UIButton b;
    b.rect.anchor = UIAnchor::TopLeft;
    b.rect.offset = {0.0f, 0.0f};
    b.rect.size   = {100.0f, 50.0f};
    const glm::vec2 inside{50.0f, 25.0f};
    const glm::vec2 outside{500.0f, 500.0f};

    SECTION("press inside, release inside -> clicked") {
        updateButtonState(b, screen, inside, /*down*/true, /*released*/false);
        REQUIRE(b.hovered);
        REQUIRE(b.pressed);
        REQUIRE_FALSE(b.clicked);
        updateButtonState(b, screen, inside, /*down*/false, /*released*/true);
        REQUIRE(b.clicked);
        REQUIRE_FALSE(b.pressed);
    }
    SECTION("press inside, release outside -> no click") {
        updateButtonState(b, screen, inside, true, false);
        updateButtonState(b, screen, outside, false, true);
        REQUIRE_FALSE(b.clicked);
        REQUIRE_FALSE(b.pressed);
    }
    SECTION("hover without press") {
        updateButtonState(b, screen, inside, false, false);
        REQUIRE(b.hovered);
        REQUIRE_FALSE(b.pressed);
        REQUIRE(b.currentColor() == b.hoverColor);
    }
}
