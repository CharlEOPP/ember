#include <catch2/catch_test_macros.hpp>
#include "ember/input/InputManager.h"
#include "ember/input/Input.h"

using namespace ember;

// All tests drive the manager headlessly via the feed*/makeActive seam — no
// GLFW window or display required.

TEST_CASE("Key edge states across three frames", "[input]") {
    InputManager im;
    im.makeActive();

    // Frame 1: key goes down.
    im.feedKey(Key::Space, true);
    im.update();
    REQUIRE(Input::isKeyPressed(Key::Space));
    REQUIRE(Input::isKeyDown(Key::Space));
    REQUIRE_FALSE(Input::isKeyReleased(Key::Space));

    // Frame 2: key still held — no longer "pressed".
    im.update();
    REQUIRE_FALSE(Input::isKeyPressed(Key::Space));
    REQUIRE(Input::isKeyDown(Key::Space));
    REQUIRE_FALSE(Input::isKeyReleased(Key::Space));

    // Frame 3: key goes up.
    im.feedKey(Key::Space, false);
    im.update();
    REQUIRE_FALSE(Input::isKeyPressed(Key::Space));
    REQUIRE_FALSE(Input::isKeyDown(Key::Space));
    REQUIRE(Input::isKeyReleased(Key::Space));

    // Frame 4: fully idle.
    im.update();
    REQUIRE_FALSE(Input::isKeyReleased(Key::Space));
    REQUIRE_FALSE(Input::isKeyDown(Key::Space));
}

TEST_CASE("Mouse buttons and position/delta/scroll", "[input]") {
    InputManager im;
    im.makeActive();

    im.feedMousePosition(100.0f, 50.0f);   // first sample primes prev
    im.update();
    REQUIRE(Input::getMousePosition() == glm::vec2(100.0f, 50.0f));
    REQUIRE(Input::getMouseDelta() == glm::vec2(0.0f, 0.0f));

    im.feedMousePosition(110.0f, 70.0f);
    im.update();
    REQUIRE(Input::getMouseDelta() == glm::vec2(10.0f, 20.0f));

    // Scroll is per-frame and resets after update.
    im.feedScroll(2.0f);
    im.update();
    REQUIRE(Input::getMouseScroll() == 2.0f);
    im.update();
    REQUIRE(Input::getMouseScroll() == 0.0f);

    im.feedMouseButton(Mouse::Left, true);
    im.update();
    REQUIRE(Input::isMouseButtonPressed(Mouse::Left));
    REQUIRE(Input::isMouseButtonDown(Mouse::Left));
    im.update();
    REQUIRE_FALSE(Input::isMouseButtonPressed(Mouse::Left));
}

TEST_CASE("Action map aggregates bindings (logical OR)", "[input]") {
    InputManager im;
    im.makeActive();
    im.loadDefaultActionMap();          // pushes "game" context
    REQUIRE(im.activeContext() == "game");

    // Jump is bound to Space (and gamepad A).
    im.feedKey(Key::Space, true);
    im.update();
    ActionState jump = Input::getAction("Jump");
    REQUIRE(jump.pressed);
    REQUIRE(jump.held);

    im.update();
    jump = Input::getAction("Jump");
    REQUIRE_FALSE(jump.pressed);
    REQUIRE(jump.held);

    // MoveX bound to D / Right with +1 scale.
    im.feedKey(Key::D, true);
    im.update();
    ActionState moveX = Input::getAction("MoveX");
    REQUIRE(moveX.held);
    REQUIRE(moveX.axisValue == 1.0f);
}

TEST_CASE("Default action map binds both movement directions", "[input]") {
    InputManager im;
    im.makeActive();
    im.loadDefaultActionMap();

    im.feedKey(Key::A, true);            // left
    im.update();
    REQUIRE(Input::getAction("MoveX").axisValue == -1.0f);

    im.feedKey(Key::A, false);
    im.feedKey(Key::D, true);            // right
    im.update();
    REQUIRE(Input::getAction("MoveX").axisValue == 1.0f);

    im.feedKey(Key::D, false);
    im.feedKey(Key::S, true);            // down
    im.update();
    REQUIRE(Input::getAction("MoveY").axisValue == -1.0f);
}

TEST_CASE("Context stack: only the top context resolves actions", "[input]") {
    InputManager im;
    im.makeActive();
    im.loadDefaultActionMap();          // "game" with Jump on Space

    // Push a "ui" context that has no Jump action.
    im.pushContext("ui");
    REQUIRE(im.activeContext() == "ui");

    im.feedKey(Key::Space, true);
    im.update();
    REQUIRE_FALSE(Input::getAction("Jump").held);   // not defined in "ui"

    im.popContext();
    REQUIRE(im.activeContext() == "game");
    REQUIRE(Input::getAction("Jump").held);          // resolves again
}
