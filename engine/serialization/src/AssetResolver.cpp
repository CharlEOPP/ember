#include "ember/serialization/AssetResolver.h"
#include "ember/serialization/ComponentSerialization.h"
#include "ember/assets/AssetManager.h"

#include <string>
#include <typeindex>

namespace ember {

void installAssetSerializationResolver(AssetManager& mgr) {
    setAssetSerializationResolver(
        [&mgr](u64 id) { return mgr.pathOf(id); },
        [&mgr](std::type_index type, const std::string& path) { return mgr.loadByType(type, path); });
}

} // namespace ember
