#pragma once
#include "ember/renderer/RHI.h"
#include <memory>
#include <string>

namespace ember {

// Loads images (PNG/JPG/BMP) via stb_image into an RHI texture. Returns the
// magenta "missing texture" placeholder on failure (never null).
class Texture2DLoader {
public:
    static std::shared_ptr<ITexture2D> load(const std::string& path);
    static std::shared_ptr<ITexture2D> missingTexture();
};

} // namespace ember
