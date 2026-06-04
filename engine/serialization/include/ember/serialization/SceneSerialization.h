#pragma once

namespace ember {

class SceneManager;

// Installs YAML scene loading into a SceneManager (call once at startup). This
// lives in the serialization module so the scene module need not depend on it
// — avoiding a scene <-> serialization dependency cycle.
void installSceneSerialization(SceneManager& manager);

} // namespace ember
