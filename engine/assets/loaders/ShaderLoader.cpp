#include "ember/assets/loaders/ShaderLoader.h"
#include "ember/renderer/ShaderLibrary.h"
#include "ember/platform/FileSystem.h"
#include "ember/core/Log.h"

namespace ember {

std::shared_ptr<Shader> ShaderLoader::load(const std::filesystem::path& path,
                                           const AssetSettings& /*settings*/) {
    auto src = FileSystem::readTextFile(path);
    if (!src) {
        EMBER_LOG_ERROR("ShaderLoader: cannot read '{}': {}", path.string(), src.error());
        return nullptr;
    }
    std::string vert, frag;
    ShaderLibrary::splitSource(*src, vert, frag);
    if (vert.empty() || frag.empty()) {
        EMBER_LOG_ERROR("ShaderLoader: '{}' missing a #type vertex/fragment stage", path.string());
        return nullptr;
    }
    auto shader = std::make_shared<Shader>();
    shader->shader = RHI::createShader(vert, frag);
    if (!shader->shader) return nullptr;
    return shader;
}

} // namespace ember
