#include <catch2/catch_test_macros.hpp>
#include "TilemapPaint.h"
#include "Commands.h"
#include "CommandHistory.h"

#include "ember/renderer/Tilemap.h"
#include "ember/scene/Scene.h"
#include "ember/ecs/World.h"

using namespace ember;

TEST_CASE("Tilemap setTile / resize", "[editor][tilemap]") {
    Tilemap tm; tm.width = 4; tm.height = 3; tm.tiles.assign(12, 0u);
    tm.setTile(1, 1, 5);
    REQUIRE(tm.at(1, 1) == 5);
    tm.setTile(99, 99, 7);              // ignored
    tm.resize(2, 2);
    REQUIRE((tm.width == 2 && tm.height == 2 && tm.at(1, 1) == 5));
    tm.resize(4, 4);
    REQUIRE((tm.at(1, 1) == 5 && tm.at(3, 3) == 0));
}

TEST_CASE("worldToCell + floodFill", "[editor][tilemap]") {
    Tilemap tm; tm.width = 4; tm.height = 4; tm.tileWorldSize = 1.0f; tm.tiles.assign(16, 0u);
    u32 cx, cy;
    REQUIRE((TilemapPaint::worldToCell(tm, {10, 20}, {10.5f, 20.5f}, cx, cy) && cx == 0 && cy == 0));
    REQUIRE_FALSE(TilemapPaint::worldToCell(tm, {10, 20}, {9.0f, 20.0f}, cx, cy));

    REQUIRE(TilemapPaint::floodFill(tm, 0, 0, 2).size() == 16);
    for (u32 y = 0; y < 4; ++y) tm.setTile(2, y, 9);          // wall at column 2
    REQUIRE(TilemapPaint::floodFill(tm, 0, 0, 2).size() == 8);
    REQUIRE(TilemapPaint::floodFill(tm, 0, 0, 0).empty());    // already target
}

TEST_CASE("paintTiles stroke undoes/redoes", "[editor][tilemap]") {
    Scene scene("t");
    Entity e = scene.create("Map");
    auto& tm = scene.world().emplace<Tilemap>(e);
    tm.width = 3; tm.height = 1; tm.tiles.assign(3, 0u);

    std::vector<Commands::TileEdit> edits = {{0,0,0,7}, {1,0,0,7}, {2,0,0,7}};
    for (const auto& ed : edits) tm.setTile(ed.x, ed.y, ed.newId);   // applied live

    CommandHistory h;
    h.pushExecuted(Commands::paintTiles(scene, e, edits));
    REQUIRE((tm.at(0,0) == 7 && tm.at(2,0) == 7));
    h.undo();
    Tilemap& tm2 = scene.world().get<Tilemap>(e);
    REQUIRE((tm2.at(0,0) == 0 && tm2.at(2,0) == 0));
    h.redo();
    REQUIRE(scene.world().get<Tilemap>(e).at(1,0) == 7);
}
