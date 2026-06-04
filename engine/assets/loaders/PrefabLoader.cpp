#include "ember/assets/loaders/PrefabLoader.h"
#include "ember/platform/FileSystem.h"
#include "ember/core/Log.h"

namespace ember {

std::shared_ptr<Prefab> PrefabLoader::load(const std::filesystem::path& path,
                                           const AssetSettings& /*settings*/) {
    auto text = FileSystem::readTextFile(path);
    if (!text) {
        EMBER_LOG_ERROR("PrefabLoader: cannot read '{}': {}", path.string(), text.error());
        return nullptr;
    }
    auto prefab = std::make_shared<Prefab>();
    prefab->yaml = *text;
    return prefab;
}

} // namespace ember
