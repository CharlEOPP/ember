#pragma once
//
// Entity — a strong alias of EnTT's entity handle.
//
// The entity width is 64-bit: ENTT_ID_TYPE=std::uint64_t is defined PUBLICly on
// the ember_ecs CMake target so every translation unit that touches EnTT agrees
// on the id type (a mismatch would be an ODR violation).
//
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>

namespace ember {

using Entity = entt::entity;

inline constexpr Entity NULL_ENTITY = entt::null;

} // namespace ember
