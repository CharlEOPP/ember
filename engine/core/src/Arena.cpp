#include "ember/core/Arena.h"

namespace ember {

ArenaAllocator::ArenaAllocator(usize capacityBytes)
    : m_buffer(std::make_unique<byte[]>(capacityBytes))
    , m_capacity(capacityBytes)
{}

void* ArenaAllocator::allocRaw(usize bytes, usize alignment) {
    // Round up m_offset to the requested alignment
    const usize aligned = (m_offset + alignment - 1) & ~(alignment - 1);
    EMBER_ASSERT(aligned + bytes <= m_capacity,
        "ArenaAllocator out of memory: requested {} bytes, {} used of {}",
        bytes, aligned, m_capacity);
    if (aligned + bytes > m_capacity) return nullptr;
    void* ptr = m_buffer.get() + aligned;
    m_offset  = aligned + bytes;
    return ptr;
}

void ArenaAllocator::reset() {
    m_offset = 0;
}

} // namespace ember
