#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ember/renderer/Tilemap.h"

using namespace ember;
using Catch::Approx;

TEST_CASE("Tileset maps tile ids to UV cells", "[renderer]") {
    Tileset ts;
    ts.columns = 4;
    ts.rows    = 4;

    REQUIRE(ts.uvForTile(0) == glm::vec4(0, 0, 0, 0));   // empty

    const glm::vec4 a = ts.uvForTile(1);                  // cell (0,0)
    REQUIRE(a.x == Approx(0.0f));
    REQUIRE(a.z == Approx(0.25f));

    const glm::vec4 b = ts.uvForTile(5);                  // cell (0,1): col 0, row 1
    REQUIRE(b.x == Approx(0.0f));
    REQUIRE(b.y == Approx(0.25f));
    REQUIRE(b.w == Approx(0.5f));
}

TEST_CASE("Tilemap indexing and solidity", "[renderer]") {
    Tilemap m;
    m.width = 3; m.height = 2;
    m.tiles = {1, 0, 2,
               0, 3, 0};
    REQUIRE(m.at(0, 0) == 1);
    REQUIRE(m.at(2, 0) == 2);
    REQUIRE(m.at(1, 1) == 3);
    REQUIRE(m.at(1, 0) == 0);
    REQUIRE(m.isSolid(0, 0));
    REQUIRE_FALSE(m.isSolid(1, 0));
    REQUIRE(m.at(99, 99) == 0);            // out of bounds
    REQUIRE_FALSE(m.isSolid(99, 99));
}
