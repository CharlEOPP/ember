#pragma once
#include "ember/renderer/RHI.h"
#include <memory>

namespace ember {

// Asset wrapper around a compiled RHI shader program.
struct Shader {
    std::shared_ptr<IShader> shader;
    IShader* rhi() const { return shader.get(); }
};

} // namespace ember
