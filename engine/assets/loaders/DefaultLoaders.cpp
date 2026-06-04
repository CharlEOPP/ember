#include "ember/assets/DefaultLoaders.h"
#include "ember/assets/AssetManager.h"
#include "ember/assets/loaders/Texture2DLoader.h"
#include "ember/assets/loaders/FontLoader.h"
#include "ember/assets/loaders/ShaderLoader.h"
#include "ember/assets/loaders/PrefabLoader.h"
#if defined(EMBER_ENABLE_AUDIO)
#include "ember/assets/loaders/AudioClipLoader.h"
#endif

#include <memory>

namespace ember {

void installDefaultLoaders(AssetManager& manager) {
    manager.registerLoader<Texture2D>(std::make_shared<Texture2DLoader>());
    manager.registerLoader<FontAsset>(std::make_shared<FontLoader>());
    manager.registerLoader<Shader>(std::make_shared<ShaderLoader>());
    manager.registerLoader<Prefab>(std::make_shared<PrefabLoader>());
#if defined(EMBER_ENABLE_AUDIO)
    manager.registerLoader<AudioClip>(std::make_shared<AudioClipLoader>());
#endif
}

} // namespace ember
