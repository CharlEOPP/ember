#include "ember/renderer/SpriteAnimationClipLoader.h"
#include "ember/assets/AssetManager.h"
#include "ember/platform/FileSystem.h"
#include "ember/core/Log.h"

#include <yaml-cpp/yaml.h>
#include <memory>

namespace ember {

std::shared_ptr<SpriteAnimationClip> SpriteAnimationClipLoader::load(
        const std::filesystem::path& path, const AssetSettings& /*settings*/) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(path.string());
    } catch (const std::exception& ex) {
        EMBER_LOG_ERROR("SpriteAnimationClipLoader: cannot parse '{}': {}", path.string(), ex.what());
        return nullptr;
    }
    auto clip = std::make_shared<SpriteAnimationClip>();
    clip->fps  = root["fps"]  ? root["fps"].as<float>(12.0f) : 12.0f;
    clip->loop = root["loop"] ? root["loop"].as<bool>(true)  : true;
    if (const YAML::Node frames = root["frames"]; frames && frames.IsSequence()) {
        for (const auto& f : frames) {
            if (f.IsSequence() && f.size() == 4)
                clip->frames.push_back({ f[0].as<float>(), f[1].as<float>(),
                                         f[2].as<float>(), f[3].as<float>() });
        }
    }
    if (clip->frames.empty())
        EMBER_LOG_WARN("SpriteAnimationClipLoader: '{}' has no frames", path.string());
    return clip;
}

void registerAnimationAssetLoaders(AssetManager& manager) {
    manager.registerLoader<SpriteAnimationClip>(std::make_shared<SpriteAnimationClipLoader>());
}

} // namespace ember
