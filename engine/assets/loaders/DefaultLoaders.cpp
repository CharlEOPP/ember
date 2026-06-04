#include "ember/assets/DefaultLoaders.h"
#include "ember/assets/AssetManager.h"
#include "ember/assets/loaders/Texture2DLoader.h"
#include "ember/assets/loaders/FontLoader.h"
#include "ember/assets/loaders/ShaderLoader.h"

#include <memory>

namespace ember {

void installDefaultLoaders(AssetManager& manager) {
    manager.registerLoader<Texture2D>(std::make_shared<Texture2DLoader>());
    manager.registerLoader<FontAsset>(std::make_shared<FontLoader>());
    manager.registerLoader<Shader>(std::make_shared<ShaderLoader>());
    // AudioClipLoader (miniaudio) is deferred — see plan Phase 4 note.
}

} // namespace ember
