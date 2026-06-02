#include <catch2/catch_test_macros.hpp>
#include "ember/core/StringHash.h"
#include <unordered_map>
#include <array>

using namespace ember;

TEST_CASE("StringHash_CompileTimeEqualsRuntime", "[stringhash]") {
    constexpr StringHash compiletime = "player"_sh;
    const StringHash runtime = StringHash::of("player");
    REQUIRE(compiletime == runtime);
}

TEST_CASE("StringHash_SameStringSameHash", "[stringhash]") {
    REQUIRE(StringHash::of("enemy") == StringHash::of("enemy"));
    REQUIRE(StringHash::of("") == StringHash::of(""));
}

TEST_CASE("StringHash_DifferentStrings", "[stringhash]") {
    constexpr std::array<const char*, 10> words = {
        "player", "enemy", "bullet", "wall", "floor",
        "camera", "light", "mesh", "shader", "texture"
    };
    for (std::size_t i = 0; i < words.size(); ++i) {
        for (std::size_t j = i + 1; j < words.size(); ++j) {
            INFO("Collision between '" << words[i] << "' and '" << words[j] << "'");
            REQUIRE(StringHash::of(words[i]) != StringHash::of(words[j]));
        }
    }
}

TEST_CASE("StringHash_IsConstexpr", "[stringhash]") {
    // Must compile as a constant expression
    static constexpr StringHash h = "constant"_sh;
    REQUIRE(h.value != 0u);
}

TEST_CASE("StringHash_HashableInUnorderedMap", "[stringhash]") {
    std::unordered_map<StringHash, int> map;
    map["health"_sh]  = 100;
    map["stamina"_sh] = 50;
    map["mana"_sh]    = 75;

    REQUIRE(map.at("health"_sh)  == 100);
    REQUIRE(map.at("stamina"_sh) == 50);
    REQUIRE(map.at("mana"_sh)    == 75);
    REQUIRE(map.size() == 3u);
}
