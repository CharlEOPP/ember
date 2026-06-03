#include <catch2/catch_test_macros.hpp>
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"

using namespace ember;

// Headless exercise: create 1000 entities with Transform + Tag and iterate them.
// (NFR-01 throughput; timing is left loose to avoid CI flakiness.)
TEST_CASE("ECS_ThousandEntitiesIterate", "[ecs][perf]") {
    World w;
    for (int i = 0; i < 1000; ++i) {
        Entity e = w.create();
        w.emplace<Transform>(e);
        w.emplace<Tag>(e);
    }
    REQUIRE(w.size() == 1000u);

    int count = 0;
    for (auto [e, tr, tag] : w.view<Transform, Tag>()) {
        (void)e; (void)tr; (void)tag;
        ++count;
    }
    REQUIRE(count == 1000);
}
