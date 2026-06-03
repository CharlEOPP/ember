#include "ember/serialization/YAMLSerializer.h"
#include "ember/scene/Scene.h"
#include "SerializationDetail.h"

#include <exception>
#include <fstream>
#include <sstream>

namespace ember {

namespace {
void applySettings(const YAML::Node& root, Scene& scene) {
    auto s = root["settings"];
    if (!s.IsDefined()) return;
    auto bg = s["backgroundColor"];
    if (bg.IsDefined())
        scene.settings().backgroundColor =
            glm::vec4{bg[0].as<float>(), bg[1].as<float>(), bg[2].as<float>(), bg[3].as<float>()};
}
} // namespace

Result<void, std::string> YAMLDeserializer::deserialize(const std::string& yaml, Scene& out) {
    detail::ensureRegistered();

    YAML::Node root;
    try {
        root = YAML::Load(yaml);
    } catch (const std::exception& ex) {
        return Err(std::string("YAML parse error: ") + ex.what());
    }

    if (!root["version"].IsDefined() || root["version"].as<int>() != 1)
        return Err(std::string("Unsupported scene version (expected 1)"));

    applySettings(root, out);

    detail::IdMap idMap;
    if (auto entities = root["entities"]; entities.IsDefined())
        detail::loadEntities(out.world(), entities, idMap);

    return {};
}

Result<void, std::string> YAMLDeserializer::loadFromFile(const std::filesystem::path& path,
                                                         Scene& out) {
    std::ifstream file(path);
    if (!file) return Err("Cannot open file for reading: " + path.string());
    std::ostringstream ss;
    ss << file.rdbuf();
    return deserialize(ss.str(), out);
}

Result<Entity, std::string> YAMLDeserializer::instantiatePrefab(Scene& scene,
                                                                const std::string& yaml,
                                                                glm::vec3 position) {
    detail::ensureRegistered();

    YAML::Node root;
    try {
        root = YAML::Load(yaml);
    } catch (const std::exception& ex) {
        return Err(std::string("YAML parse error: ") + ex.what());
    }

    if (!root["version"].IsDefined() || root["version"].as<int>() != 1)
        return Err(std::string("Unsupported prefab version (expected 1)"));
    if (!root["entities"].IsDefined())
        return Err(std::string("Prefab has no entities"));

    detail::IdMap idMap;
    detail::loadEntities(scene.world(), root["entities"], idMap);

    Entity newRoot = NULL_ENTITY;
    if (root["prefabRoot"].IsDefined()) {
        auto it = idMap.find(root["prefabRoot"].as<std::uint64_t>());
        if (it != idMap.end()) newRoot = it->second;
    }
    if (newRoot == NULL_ENTITY)
        return Err(std::string("Prefab root entity not found"));

    if (Transform* tr = scene.world().tryGet<Transform>(newRoot)) tr->position = position;
    scene.markTransformDirty(newRoot);
    return newRoot;
}

} // namespace ember
