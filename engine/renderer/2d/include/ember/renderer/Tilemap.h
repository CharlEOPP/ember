#pragma once
#include "ember/core/Types.h"
#include "ember/assets/AssetHandle.h"
#include "ember/assets/Texture2D.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace ember {

// An atlas of equal-size tiles. Tile id 0 = empty; id >= 1 maps to atlas cell
// (id-1), left-to-right, top-to-bottom.
struct Tileset {
    AssetHandle<Texture2D> texture;
    std::string            texturePath;
    u32 columns = 1;
    u32 rows    = 1;

    [[nodiscard]] glm::vec4 uvForTile(u32 id) const {
        if (id == 0 || columns == 0 || rows == 0) return {0.0f, 0.0f, 0.0f, 0.0f};
        const u32 i   = id - 1;
        const u32 col = i % columns;
        const u32 row = i / columns;
        const f32 uw  = 1.0f / static_cast<f32>(columns);
        const f32 vh  = 1.0f / static_cast<f32>(rows);
        const f32 u0  = static_cast<f32>(col) * uw;
        const f32 v0  = static_cast<f32>(row) * vh;
        return {u0, v0, u0 + uw, v0 + vh};
    }
};

// A grid of tile ids (row-major). Loaded from a .tilemap file or built in code.
// Not serialized inline in scenes (referenced by asset path — deferred); attach
// as a component to an entity whose Transform places the map's origin.
struct Tilemap {
    u32 width  = 0;
    u32 height = 0;
    f32 tileWorldSize = 1.0f;
    Tileset tileset;
    std::vector<u32> tiles;   // size width*height, 0 = empty
    i32 layer = 0;

    [[nodiscard]] u32 at(u32 x, u32 y) const {
        return (x < width && y < height) ? tiles[static_cast<usize>(y) * width + x] : 0u;
    }
    [[nodiscard]] bool isSolid(u32 x, u32 y) const { return at(x, y) != 0u; }

    // Write a tile id at (x,y); ignored if out of bounds. (Editor painting.)
    void setTile(u32 x, u32 y, u32 id) {
        if (x < width && y < height) tiles[static_cast<usize>(y) * width + x] = id;
    }

    // Resize the grid, preserving overlapping cells; new cells are empty (0).
    void resize(u32 newW, u32 newH) {
        std::vector<u32> next(static_cast<usize>(newW) * newH, 0u);
        const u32 cw = newW < width ? newW : width;
        const u32 ch = newH < height ? newH : height;
        for (u32 y = 0; y < ch; ++y)
            for (u32 x = 0; x < cw; ++x)
                next[static_cast<usize>(y) * newW + x] = tiles[static_cast<usize>(y) * width + x];
        tiles = std::move(next);
        width = newW;
        height = newH;
    }
};

} // namespace ember
