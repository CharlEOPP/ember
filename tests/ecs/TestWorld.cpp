#include <catch2/catch_test_macros.hpp>
#include "ember/ecs/World.h"

#include <entt/entt.hpp>

using namespace ember;

namespace {
struct Pos { f32 x = 0.0f, y = 0.0f; };
struct Vel { f32 v = 0.0f; };
struct Marker {};

// Listener for EnTT-native lifecycle hooks (member fn matches the signal signature).
struct ConstructCounter {
    int count = 0;
    void onAdd(entt::registry&, entt::entity) { ++count; }
};
} // namespace

TEST_CASE("World_CreateDestroy", "[ecs]") {
    World w;
    Entity a = w.create();
    Entity b = w.create();
    REQUIRE(w.valid(a));
    REQUIRE(w.valid(b));
    REQUIRE(w.size() == 2u);

    w.destroy(a);
    REQUIRE_FALSE(w.valid(a));
    REQUIRE(w.valid(b));
    REQUIRE(w.size() == 1u);
}

TEST_CASE("World_ComponentCRUD", "[ecs]") {
    World w;
    Entity e = w.create();

    Pos& p = w.emplace<Pos>(e, 1.0f, 2.0f);
    REQUIRE(p.x == 1.0f);
    REQUIRE(w.has<Pos>(e));
    REQUIRE(w.get<Pos>(e).y == 2.0f);

    REQUIRE(w.tryGet<Vel>(e) == nullptr);   // absent
    w.emplace<Vel>(e, 5.0f);
    REQUIRE(w.tryGet<Vel>(e) != nullptr);
    REQUIRE((w.has<Pos, Vel>(e)));          // all-of

    w.remove<Pos>(e);
    REQUIRE_FALSE(w.has<Pos>(e));
    REQUIRE(w.has<Vel>(e));
}

TEST_CASE("World_ViewIteration", "[ecs]") {
    World w;
    for (int i = 0; i < 3; ++i) {            // 3 with Pos+Vel
        Entity e = w.create();
        w.emplace<Pos>(e);
        w.emplace<Vel>(e);
    }
    Entity only = w.create();                // 1 with Pos only
    w.emplace<Pos>(only);

    int both = 0;
    for (auto [e, pos, vel] : w.view<Pos, Vel>()) {
        (void)e; (void)pos; (void)vel;
        ++both;
    }
    REQUIRE(both == 3);
}

TEST_CASE("World_LifecycleHooks", "[ecs]") {
    World w;
    ConstructCounter counter;
    w.on_construct<Marker>().connect<&ConstructCounter::onAdd>(counter);

    Entity a = w.create();
    Entity b = w.create();
    w.emplace<Marker>(a);
    w.emplace<Marker>(b);
    REQUIRE(counter.count == 2);
}
