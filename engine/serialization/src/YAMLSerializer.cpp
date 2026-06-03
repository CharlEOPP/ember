#include "ember/serialization/YAMLSerializer.h"
#include "ember/scene/Scene.h"
#include "SerializationDetail.h"

#include <fstream>

namespace ember {

std::string YAMLSerializer::serialize(const Scene& scene) {
    detail::ensureRegistered();
    World& world = const_cast<World&>(scene.world());   // read-only iteration

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "version" << YAML::Value << 1;
    out << YAML::Key << "name"    << YAML::Value << scene.name();

    const glm::vec4& bg = scene.settings().backgroundColor;
    out << YAML::Key << "settings" << YAML::Value << YAML::BeginMap
        << YAML::Key << "backgroundColor" << YAML::Value
        << YAML::Flow << YAML::BeginSeq << bg.x << bg.y << bg.z << bg.w << YAML::EndSeq
        << YAML::EndMap;

    out << YAML::Key << "entities" << YAML::Value << YAML::BeginSeq;
    for (auto [e, tag] : world.view<Tag>()) {
        (void)tag;
        detail::emitEntity(out, world, e);
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;

    return std::string(out.c_str());
}

Result<void, std::string> YAMLSerializer::saveToFile(const Scene& scene,
                                                     const std::filesystem::path& path) {
    std::ofstream file(path);
    if (!file) return Err("Cannot open file for writing: " + path.string());
    file << serialize(scene);
    return {};
}

std::string YAMLSerializer::serializePrefab(Scene& scene, Entity root) {
    detail::ensureRegistered();
    World& world = scene.world();

    std::vector<Entity> subtree;
    detail::collectSubtree(world, root, subtree);

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "version"    << YAML::Value << 1;
    out << YAML::Key << "prefabRoot" << YAML::Value << detail::idOf(root);
    out << YAML::Key << "entities"   << YAML::Value << YAML::BeginSeq;
    for (Entity e : subtree) detail::emitEntity(out, world, e);
    out << YAML::EndSeq;
    out << YAML::EndMap;

    return std::string(out.c_str());
}

Result<void, std::string> YAMLSerializer::savePrefabToFile(Scene& scene, Entity root,
                                                           const std::filesystem::path& path) {
    std::ofstream file(path);
    if (!file) return Err("Cannot open file for writing: " + path.string());
    file << serializePrefab(scene, root);
    return {};
}

} // namespace ember
