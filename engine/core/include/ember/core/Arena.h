#pragma once
#include "ember/core/Types.h"
#include "ember/core/Assert.h"
#include <memory>
#include <new>

namespace ember {

/**
 * Linear (bump-pointer) allocator for frame-lifetime objects.
 * Reset each frame — does NOT call destructors on reset().
 * Not thread-safe by design (one arena per thread or scope).
 */
class ArenaAllocator {
public:
    explicit ArenaAllocator(usize capacityBytes);
    ~ArenaAllocator() = default;

    ArenaAllocator(const ArenaAllocator&)            = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;
    ArenaAllocator(ArenaAllocator&&)                 = default;
    ArenaAllocator& operator=(ArenaAllocator&&)      = default;

    /** Construct a T in the arena. Returns nullptr and asserts if full. */
    template<typename T, typename... Args>
    T* alloc(Args&&... args) {
        void* ptr = allocRaw(sizeof(T), alignof(T));
        if (!ptr) return nullptr;
        return new(ptr) T(std::forward<Args>(args)...);
    }

    /** Allocate raw aligned memory. Returns nullptr and asserts if full. */
    void* allocRaw(usize bytes, usize alignment = alignof(std::max_align_t));

    /** Reset bump pointer to start. Does NOT call destructors. */
    void reset();

    usize used()     const noexcept { return m_offset; }
    usize capacity() const noexcept { return m_capacity; }

private:
    std::unique_ptr<byte[]> m_buffer;
    usize                   m_capacity;
    usize                   m_offset = 0;
};

} // namespace ember
