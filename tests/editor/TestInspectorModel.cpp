#include <catch2/catch_test_macros.hpp>
#include "InspectorRegistry.h"

#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"
#include "ember/ecs/Reflect.h"
#include "ember/renderer/Components2D.h"

#include <string>
#include <vector>

using namespace ember;

namespace {
// A non-ImGui visitor mirroring the inspector visitor's shape — records which
// fields reflection exposes (TST-04: the inspect model visits the right fields).
struct RecordingVisitor {
    std::vector<std::string> names;
    template<typename T> void operator()(const char* n, T&) { names.emplace_back(n); }
};
} // namespace

TEST_CASE("Reflection visits a component's declared fields", "[editor][inspector]") {
    Transform t;
    RecordingVisitor v;
    reflect(t, v);
    REQUIRE(v.names == std::vector<std::string>{"position", "rotation", "scale"});
}

TEST_CASE("InspectorRegistry add-list = registered components not present", "[editor][inspector]") {
    auto& reg = InspectorRegistry::instance();
    reg.clear();
    auto noop = [](World&, Entity) { return false; };
    registerInspector<Transform>("Transform", noop);
    registerInspector<SpriteRenderer>("SpriteRenderer", noop);
    REQUIRE(reg.all().size() == 2);
    registerInspector<Transform>("Transform", noop);   // idempotent by type
    REQUIRE(reg.all().size() == 2);

    World w;
    Entity e = w.create();
    w.emplace<Transform>(e);

    const InspectorEntry* tr = reg.byType(typeid(Transform));
    const InspectorEntry* sr = reg.byType(typeid(SpriteRenderer));
    REQUIRE(tr);
    REQUIRE(sr);
    REQUIRE(tr->has(w, e));
    REQUIRE_FALSE(sr->has(w, e));

    int absent = 0;
    for (const auto& en : reg.all()) if (!en.has(w, e)) ++absent;
    REQUIRE(absent == 1);   // only SpriteRenderer

    sr->add(w, e);
    REQUIRE(sr->has(w, e));
    sr->remove(w, e);
    REQUIRE_FALSE(sr->has(w, e));
}

TEST_CASE("InspectorEntry snapshot/restore copies the component value", "[editor][inspector]") {
    auto& reg = InspectorRegistry::instance();
    reg.clear();
    registerInspector<Transform>("Transform", [](World&, Entity) { return false; });

    World w;
    Entity e = w.create();
    auto& t = w.emplace<Transform>(e);
    t.position = {1.0f, 2.0f, 3.0f};

    const InspectorEntry* tr = reg.byType(typeid(Transform));
    auto snap = tr->snapshot(w, e);     // capture {1,2,3}
    t.position = {9.0f, 9.0f, 9.0f};     // mutate
    tr->restore(w, e, snap);            // restore
    REQUIRE(w.get<Transform>(e).position == glm::vec3(1.0f, 2.0f, 3.0f));
}
