#pragma once
#include "ember/core/Types.h"
#include <string_view>
#include <functional>

namespace ember {

struct StringHash {
    u64 value = 0;

    constexpr StringHash() = default;
    constexpr explicit StringHash(u64 v) : value(v) {}

    constexpr bool operator==(const StringHash&) const noexcept = default;
    constexpr bool operator!=(const StringHash&) const noexcept = default;

    // FNV-1a 64-bit
    static constexpr u64 fnv1a(std::string_view str) noexcept {
        u64 hash = 14695981039346656037ULL;
        for (const char c : str) {
            hash ^= static_cast<u64>(static_cast<unsigned char>(c));
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    static constexpr StringHash of(std::string_view str) noexcept {
        return StringHash{ fnv1a(str) };
    }
};

constexpr StringHash operator""_sh(const char* str, std::size_t len) noexcept {
    return StringHash::of(std::string_view(str, len));
}

} // namespace ember

// std::hash specialisation — makes StringHash usable in unordered_map/set
template<>
struct std::hash<ember::StringHash> {
    std::size_t operator()(const ember::StringHash& sh) const noexcept {
        return static_cast<std::size_t>(sh.value);
    }
};
