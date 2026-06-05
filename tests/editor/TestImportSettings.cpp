#include <catch2/catch_test_macros.hpp>
#include "ember/assets/ImportSettings.h"

using namespace ember;

TEST_CASE("TextureImportSettings round-trips through AssetSettings", "[editor][import]") {
    TextureImportSettings t;
    t.filter = TextureFilter::Nearest;
    t.wrap   = TextureWrap::ClampToEdge;
    t.format = TextureFormat::R8;
    t.generateMips = true;
    t.maxSize = 1024;

    TextureImportSettings u = TextureImportSettings::fromSettings(t.toSettings());
    REQUIRE(u.filter == TextureFilter::Nearest);
    REQUIRE(u.wrap   == TextureWrap::ClampToEdge);
    REQUIRE(u.format == TextureFormat::R8);
    REQUIRE(u.generateMips);
    REQUIRE(u.maxSize == 1024);

    TextureImportSettings d = TextureImportSettings::fromSettings(TextureImportSettings{}.toSettings());
    REQUIRE(d.format == TextureFormat::RGBA8);
    REQUIRE(d.filter == TextureFilter::Linear);
    REQUIRE_FALSE(d.generateMips);
}

TEST_CASE("FontImportSettings round-trips its sizes", "[editor][import]") {
    FontImportSettings f; f.sizes = {10, 20, 30};
    FontImportSettings u = FontImportSettings::fromSettings(f.toSettings());
    REQUIRE(u.sizes.size() == 3);
    REQUIRE(u.sizes[0] == 10);
    REQUIRE(u.sizes[2] == 30);
}
