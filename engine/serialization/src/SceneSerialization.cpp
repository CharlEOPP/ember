#include "ember/serialization/SceneSerialization.h"
#include "ember/serialization/YAMLSerializer.h"
#include "ember/scene/SceneManager.h"
#include "ember/scene/Scene.h"

#include <filesystem>

namespace ember {

void installSceneSerialization(SceneManager& manager) {
    manager.setLoader([](const std::filesystem::path& path, Scene& out) {
        return YAMLDeserializer::loadFromFile(path, out);
    });
}

} // namespace ember
