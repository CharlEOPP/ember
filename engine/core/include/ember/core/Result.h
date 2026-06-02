#pragma once

// Use std::expected if available (GCC 12+, Clang 16+, MSVC 19.36+),
// otherwise fall back to the vendored tl::expected.
#if __has_include(<expected>) && defined(__cpp_lib_expected)
#  include <expected>
   namespace ember {
       template<typename T, typename E> using Result = std::expected<T, E>;
       template<typename E> auto Err(E&& e) { return std::unexpected(std::forward<E>(e)); }
   }
#else
#  include <expected.hpp>
   namespace ember {
       template<typename T, typename E> using Result = tl::expected<T, E>;
       template<typename E> auto Err(E&& e) { return tl::unexpected<E>(std::forward<E>(e)); }
   }
#endif
