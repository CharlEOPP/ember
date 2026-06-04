#pragma once
#include "ember/assets/IAssetLoader.h"
#include "ember/renderer/Tilemap.h"
#include <memory>

namespace ember {

class AssetManager;

// Loads a Tilemap from a simple text format:
//   width 8
//   height 4
//   tilesize 1.0
//   tileset tilesets/world.png
//   columns 4
//   rows 4
//   tiles
//   1 1 0 2
//   0 0 0 0
//   ...
class TilemapLoader : public IAssetLoader<Tilemap> {
public:
    std::shared_ptr<Tilemap> load(const std::filesystem::path& path,
                                  const AssetSettings& settings) override;
};

void registerTilemapLoader(AssetManager& manager);

} // namespace ember
