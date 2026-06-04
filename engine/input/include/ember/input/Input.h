#pragma once
#include "ember/input/Keys.h"
#include "ember/input/InputAction.h"
#include <glm/glm.hpp>
#include <string>

// Static facade over the active InputManager. All calls are null-safe: if no
// manager is active, queries report "nothing pressed" / zero. This is the API
// game code should use; it never sees GLFW or even the InputManager type.

namespace ember::Input {

bool isKeyDown(Key k);
bool isKeyPressed(Key k);
bool isKeyReleased(Key k);

bool isMouseButtonDown(Mouse m);
bool isMouseButtonPressed(Mouse m);
bool isMouseButtonReleased(Mouse m);

glm::vec2 getMousePosition();
glm::vec2 getMouseDelta();
f32       getMouseScroll();

f32  getGamepadAxis(GamepadAxis a);
bool isGamepadButtonDown(GamepadButton b);

ActionState getAction(const std::string& name);

} // namespace ember::Input
