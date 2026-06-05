#include <catch2/catch_test_macros.hpp>
#include "Selection.h"

using namespace ember;

namespace { Entity E(int i) { return static_cast<Entity>(i); } }

TEST_CASE("Selection select/add/toggle/primary", "[editor][selection]") {
    Selection s;
    REQUIRE(s.empty());
    REQUIRE(s.primary() == NULL_ENTITY);

    s.selectOnly(E(1));
    REQUIRE(s.size() == 1);
    REQUIRE(s.contains(E(1)));
    REQUIRE(s.primary() == E(1));

    s.add(E(2));
    REQUIRE(s.size() == 2);
    REQUIRE(s.primary() == E(2));

    s.toggle(E(1));                 // remove 1; primary (2) stays
    REQUIRE_FALSE(s.contains(E(1)));
    REQUIRE(s.primary() == E(2));

    s.toggle(E(3));                 // add 3 -> primary
    REQUIRE(s.contains(E(3)));
    REQUIRE(s.primary() == E(3));

    s.remove(E(3));                 // primary reassigned to remaining member
    REQUIRE(s.primary() == E(2));

    s.selectOnly(NULL_ENTITY);      // clears
    REQUIRE(s.empty());
    REQUIRE(s.primary() == NULL_ENTITY);
}
