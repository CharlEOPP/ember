#pragma once

namespace ember {

// Register the renderer's serializable components (SpriteRenderer, Camera2D)
// with the scene serializer's ComponentRegistry. Call once at startup before
// loading/saving scenes that contain them. Idempotent.
void registerRenderComponents();

} // namespace ember
