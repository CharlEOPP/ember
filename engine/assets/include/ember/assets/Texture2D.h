#pragma once
#include "ember/core/Types.h"
#include "ember/renderer/RHI.h"
#include <memory>

namespace ember {

// Asset wrapper around an RHI texture handle, so `AssetManager::get<Texture2D>`
// yields a bindable texture plus its dimensions. The GPU resource lifetime is
// the wrapped shared_ptr's.
struct Texture2D {
    std::shared_ptr<ITexture2D> texture;
    u32 width  = 0;
    u32 height = 0;

    ITexture2D* rhi() const { return texture.get(); }
};

} // namespace ember
