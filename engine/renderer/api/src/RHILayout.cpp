#include "ember/renderer/RHI.h"
#include "ember/core/Assert.h"

namespace ember {

u32 shaderDataTypeSize(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:  return 4;
        case ShaderDataType::Float2: return 4 * 2;
        case ShaderDataType::Float3: return 4 * 3;
        case ShaderDataType::Float4: return 4 * 4;
        case ShaderDataType::Int:    return 4;
        case ShaderDataType::Int2:   return 4 * 2;
        case ShaderDataType::Int3:   return 4 * 3;
        case ShaderDataType::Int4:   return 4 * 4;
        case ShaderDataType::Mat3:   return 4 * 3 * 3;
        case ShaderDataType::Mat4:   return 4 * 4 * 4;
        case ShaderDataType::Bool:   return 1;
        case ShaderDataType::None:   break;
    }
    EMBER_ASSERT(false, "Unknown ShaderDataType");
    return 0;
}

u32 shaderDataTypeComponentCount(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:  return 1;
        case ShaderDataType::Float2: return 2;
        case ShaderDataType::Float3: return 3;
        case ShaderDataType::Float4: return 4;
        case ShaderDataType::Int:    return 1;
        case ShaderDataType::Int2:   return 2;
        case ShaderDataType::Int3:   return 3;
        case ShaderDataType::Int4:   return 4;
        case ShaderDataType::Mat3:   return 3 * 3;
        case ShaderDataType::Mat4:   return 4 * 4;
        case ShaderDataType::Bool:   return 1;
        case ShaderDataType::None:   break;
    }
    EMBER_ASSERT(false, "Unknown ShaderDataType");
    return 0;
}

BufferLayout::BufferLayout(std::initializer_list<BufferElement> elements)
    : m_elements(elements) {
    u32 offset = 0;
    m_stride = 0;
    for (auto& e : m_elements) {
        e.size   = shaderDataTypeSize(e.type);
        e.offset = offset;
        offset    += e.size;
        m_stride  += e.size;
    }
}

} // namespace ember
