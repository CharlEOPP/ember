#pragma once
#include "ember/core/Types.h"

namespace ember {

using UUID = u64;
inline constexpr UUID NULL_UUID = 0;

/** Generate a unique non-zero 64-bit ID. Thread-safe. */
UUID generateUUID();

} // namespace ember
