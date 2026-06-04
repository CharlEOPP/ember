#pragma once

namespace ember {

// Registers the built-in component inspectors (reflection defaults + hand-written
// overrides for Transform/SpriteRenderer/Camera2D). Call once at editor startup.
void registerEditorInspectors();

} // namespace ember
