#pragma once
#include "ember/core/Types.h"
#include <functional>

namespace ember {

// Lightweight, copyable handle to a managed asset. The `id` indexes an entry in
// the AssetManager; `0` is the null handle. The template parameter `T` exists
// only for compile-time type safety — the runtime payload is `id`.
template<typename T>
struct AssetHandle {
    u64 id = 0;

    static constexpr AssetHandle null() { return AssetHandle{0}; }
    constexpr bool valid() const { return id != 0; }
    explicit constexpr operator bool() const { return valid(); }

    constexpr bool operator==(const AssetHandle& o) const { return id == o.id; }
    constexpr bool operator!=(const AssetHandle& o) const { return id != o.id; }
};

} // namespace ember

// Hashable so handles can key unordered containers.
namespace std {
template<typename T>
struct hash<ember::AssetHandle<T>> {
    size_t operator()(const ember::AssetHandle<T>& h) const noexcept {
        return std::hash<ember::u64>{}(h.id);
    }
};
} // namespace std
