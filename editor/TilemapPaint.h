#pragma once
#include "ember/core/Types.h"
#include "ember/renderer/Tilemap.h"

#include <glm/glm.hpp>
#include <cmath>
#include <utility>
#include <vector>

// ImGui-free tilemap paint helpers (unit-testable in ember_editor_core).
namespace ember::TilemapPaint {

// Map a world point to a tile cell. Returns false if outside the grid.
inline bool worldToCell(const Tilemap& tm, const glm::vec2& mapOrigin,
                        const glm::vec2& world, u32& outX, u32& outY) {
    if (tm.tileWorldSize <= 0.0f || tm.width == 0 || tm.height == 0) return false;
    const float fx = (world.x - mapOrigin.x) / tm.tileWorldSize;
    const float fy = (world.y - mapOrigin.y) / tm.tileWorldSize;
    if (fx < 0.0f || fy < 0.0f) return false;
    const u32 x = static_cast<u32>(std::floor(fx));
    const u32 y = static_cast<u32>(std::floor(fy));
    if (x >= tm.width || y >= tm.height) return false;
    outX = x; outY = y;
    return true;
}

// 4-connected flood fill from (sx,sy): the contiguous region sharing the start
// id. Returns the cells to change (empty if the start already holds `newId`).
inline std::vector<std::pair<u32, u32>> floodFill(const Tilemap& tm, u32 sx, u32 sy, u32 newId) {
    std::vector<std::pair<u32, u32>> out;
    if (sx >= tm.width || sy >= tm.height) return out;
    const u32 target = tm.at(sx, sy);
    if (target == newId) return out;

    std::vector<char> visited(static_cast<usize>(tm.width) * tm.height, 0);
    std::vector<std::pair<u32, u32>> stack{{sx, sy}};
    visited[static_cast<usize>(sy) * tm.width + sx] = 1;
    while (!stack.empty()) {
        auto [x, y] = stack.back(); stack.pop_back();
        if (tm.at(x, y) != target) continue;
        out.push_back({x, y});
        const std::pair<int, int> nbr[4] = {{1,0},{-1,0},{0,1},{0,-1}};
        for (auto [dx, dy] : nbr) {
            const int nx = static_cast<int>(x) + dx, ny = static_cast<int>(y) + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<int>(tm.width) || ny >= static_cast<int>(tm.height))
                continue;
            const usize idx = static_cast<usize>(ny) * tm.width + static_cast<usize>(nx);
            if (!visited[idx] && tm.at(static_cast<u32>(nx), static_cast<u32>(ny)) == target) {
                visited[idx] = 1;
                stack.push_back({static_cast<u32>(nx), static_cast<u32>(ny)});
            }
        }
    }
    return out;
}

} // namespace ember::TilemapPaint
