#pragma once
#include <string>

namespace ember {

// A serialized entity sub-tree (YAML text), loaded as an asset. Spawned into a
// scene via YAMLDeserializer::instantiatePrefab (see ScriptComponent::instantiate).
// Kept as plain text here so the assets module needs no serialization dependency.
struct Prefab {
    std::string yaml;
};

} // namespace ember
