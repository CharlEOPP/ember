#pragma once
#include "ember/assets/IAssetLoader.h"
#include "ember/assets/Shader.h"
#include <memory>

namespace ember {

// Loads a combined "#type vertex / #type fragment" GLSL file and compiles it
// into a Shader asset via the RHI.
class ShaderLoader : public IAssetLoader<Shader> {
public:
    std::shared_ptr<Shader> load(const std::filesystem::path& path,
                                 const AssetSettings& settings) override;
};

} // namespace ember
