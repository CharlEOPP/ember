#pragma once
#include "ember/ecs/Reflect.h"

#include <string>

namespace ember {

// Tags an entity as an instance of a prefab asset (the `.eprefab` it was created
// from). The editor uses it to highlight instances and offer "Revert to Prefab".
// Serialized with the scene so the link survives save/load.
struct PrefabInstance {
    std::string source;   // virtual/relative path to the .eprefab
};

EMBER_REFLECT(PrefabInstance) {
    EMBER_FIELD(source);
}

} // namespace ember
