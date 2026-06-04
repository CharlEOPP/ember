#pragma once
#include "ember/assets/IAssetLoader.h"
#include "ember/renderer/SpriteAnimation.h"
#include <memory>

namespace ember {

class AssetManager;

// Loads a SpriteAnimationClip from a YAML file:
//   fps: 12
//   loop: true
//   frames: [ [0,0,0.25,1], [0.25,0,0.5,1], ... ]
class SpriteAnimationClipLoader : public IAssetLoader<SpriteAnimationClip> {
public:
    std::shared_ptr<SpriteAnimationClip> load(const std::filesystem::path& path,
                                              const AssetSettings& settings) override;
};

// Registers the SpriteAnimationClip loader with the manager.
void registerAnimationAssetLoaders(AssetManager& manager);

} // namespace ember
