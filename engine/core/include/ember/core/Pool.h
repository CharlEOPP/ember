#pragma once
#include "ember/core/Types.h"
#include "ember/core/Assert.h"
#include <memory>
#include <typeinfo>

namespace ember {

/**
 * Fixed-capacity pool allocator for a single type T.
 * O(1) alloc and free via a free-list embedded in unused slots.
 */
template<typename T>
class PoolAllocator {
public:
    explicit PoolAllocator(usize capacity)
        : m_capacity(capacity)
        , m_storage(std::make_unique<Slot[]>(capacity))
    {
        // Build the initial free list
        m_freeHead = 0;
        for (usize i = 0; i < capacity - 1; ++i)
            m_storage[i].nextFree = static_cast<u32>(i + 1);
        m_storage[capacity - 1].nextFree = INVALID;
    }

    PoolAllocator(const PoolAllocator&)            = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    template<typename... Args>
    [[nodiscard]] T* alloc(Args&&... args) {
        EMBER_ASSERT(m_freeHead != INVALID,
            "PoolAllocator<{}> is full ({} slots)", typeid(T).name(), m_capacity);
        if (m_freeHead == INVALID) return nullptr;

        const u32 idx = m_freeHead;
        m_freeHead    = m_storage[idx].nextFree;
        ++m_size;
        return new (m_storage[idx].data) T(std::forward<Args>(args)...);
    }

    void free(T* ptr) {
        EMBER_ASSERT(ptr != nullptr, "PoolAllocator::free called with nullptr");
        ptr->~T();
        // Derive the slot index from the pointer
        auto* slot = reinterpret_cast<Slot*>(ptr);
        const u32 idx    = static_cast<u32>(slot - m_storage.get());
        slot->nextFree   = m_freeHead;
        m_freeHead       = idx;
        --m_size;
    }

    usize size()     const noexcept { return m_size; }
    usize capacity() const noexcept { return m_capacity; }

private:
    static constexpr u32 INVALID = ~u32{0};

    union Slot {
        alignas(T) byte data[sizeof(T)];
        u32 nextFree;
    };

    usize                   m_capacity;
    usize                   m_size     = 0;
    u32                     m_freeHead = INVALID;
    std::unique_ptr<Slot[]> m_storage;
};

} // namespace ember
