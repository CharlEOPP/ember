#pragma once
#include "ember/assets/IAssetLoader.h"
#include "ember/assets/Prefab.h"
#include <memory>

namespace ember {

// Loads a .prefab / .escene file's text into a Prefab asset (no parsing here;
// instantiation parses the YAML on demand).
class PrefabLoader : public IAssetLoader<Prefab> {
public:
    std::shared_ptr<Prefab> load(const std::filesystem::path& path,
                                 const AssetSettings& settings) override;
};

} // namespace ember
