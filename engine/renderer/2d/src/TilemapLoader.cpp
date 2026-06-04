#include "ember/renderer/TilemapLoader.h"
#include "ember/assets/AssetManager.h"
#include "ember/platform/FileSystem.h"
#include "ember/core/Log.h"

#include <memory>
#include <sstream>
#include <string>

namespace ember {

std::shared_ptr<Tilemap> TilemapLoader::load(const std::filesystem::path& path,
                                             const AssetSettings& /*settings*/) {
    auto text = FileSystem::readTextFile(path);
    if (!text) {
        EMBER_LOG_ERROR("TilemapLoader: cannot read '{}': {}", path.string(), text.error());
        return nullptr;
    }
    auto map = std::make_shared<Tilemap>();
    std::istringstream in(*text);
    std::string key;
    while (in >> key) {
        if      (key == "width")    in >> map->width;
        else if (key == "height")   in >> map->height;
        else if (key == "tilesize") in >> map->tileWorldSize;
        else if (key == "tileset")  in >> map->tileset.texturePath;
        else if (key == "columns")  in >> map->tileset.columns;
        else if (key == "rows")     in >> map->tileset.rows;
        else if (key == "tiles") {
            map->tiles.reserve(static_cast<usize>(map->width) * map->height);
            u32 v;
            while (in >> v) map->tiles.push_back(v);
            break;
        }
    }
    if (map->tiles.size() != static_cast<usize>(map->width) * map->height)
        EMBER_LOG_WARN("TilemapLoader: '{}' tile count {} != {}x{}",
                       path.string(), map->tiles.size(), map->width, map->height);
    return map;
}

void registerTilemapLoader(AssetManager& manager) {
    manager.registerLoader<Tilemap>(std::make_shared<TilemapLoader>());
}

} // namespace ember
