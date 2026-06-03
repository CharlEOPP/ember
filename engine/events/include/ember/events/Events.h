#pragma once
#include "ember/core/Types.h"
#include <string>

namespace ember {

// ---- Window events (supersede the stub structs from Epic 02's platform/Event.h) ----
struct WindowResizeEvent { u32 width = 0, height = 0; };
struct WindowCloseEvent  {};

// ---- Input events (types only; produced by the Input epic, not this one) ----
struct KeyPressedEvent   { i32 key = 0; i32 mods = 0; bool repeat = false; };
struct KeyReleasedEvent  { i32 key = 0; i32 mods = 0; };
struct MouseMovedEvent   { f32 x = 0.0f, y = 0.0f; };
struct MouseButtonEvent  { i32 button = 0; i32 action = 0; i32 mods = 0; };
struct MouseScrollEvent  { f32 xOffset = 0.0f, yOffset = 0.0f; };

// ---- Scene events ----
struct SceneLoadedEvent   { std::string name; };
struct SceneUnloadedEvent { std::string name; };

} // namespace ember
