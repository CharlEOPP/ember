#pragma once
//
// YAML scene/prefab serialization. yaml-cpp is an implementation detail and never
// appears in this header (NFR-04 / SER-11).
//
#include "ember/core/Result.h"
#include "ember/ecs/Entity.h"

#include <glm/glm.hpp>
#include <filesystem>
#include <string>

namespace ember {

class Scene;

struct YAMLSerializer {
    // Serialize an entire scene to a YAML string (SER-01).
    static std::string serialize(const Scene& scene);
    static Result<void, std::string> saveToFile(const Scene& scene, const std::filesystem::path& path);

    // Serialize a single entity sub-tree as a prefab (SER-10).
    static std::string serializePrefab(Scene& scene, Entity root);
    static Result<void, std::string> savePrefabToFile(Scene& scene, Entity root,
                                                      const std::filesystem::path& path);
};

struct YAMLDeserializer {
    // Recreate entities/components from YAML into an (empty) scene (SER-02).
    static Result<void, std::string> deserialize(const std::string& yaml, Scene& out);
    static Result<void, std::string> loadFromFile(const std::filesystem::path& path, Scene& out);

    // Instantiate a prefab sub-tree into the scene, positioning its root (SER-10).
    static Result<Entity, std::string> instantiatePrefab(Scene& scene, const std::string& yaml,
                                                         glm::vec3 position);
};

} // namespace ember
