#include <catch2/catch_test_macros.hpp>
#include "ember/ecs/Components.h"
#include "ember/ecs/ComponentRegistry.h"
#include "ember/core/Assert.h"

#include <string>
#include <vector>

using namespace ember;

namespace {
// A visitor that just records the field names it is shown.
struct Recorder {
    std::vector<std::string> names;
    template<typename T> void operator()(const char* name, T&) { names.emplace_back(name); }
};
} // namespace

TEST_CASE("Reflection_FieldWalk", "[ecs][reflect]") {
    Transform t;
    Recorder rec;
    reflect(t, rec);
    REQUIRE(rec.names == std::vector<std::string>{"position", "rotation", "scale"});

    Tag tag;
    Recorder rec2;
    reflect(tag, rec2);
    REQUIRE(rec2.names == std::vector<std::string>{"name", "layer"});
}

TEST_CASE("Reflection_RegistryLookup", "[ecs][reflect]") {
    auto& reg = ComponentRegistry::instance();
    reg.clear();

    ComponentMeta meta;
    meta.name = "Foo";
    meta.type = std::type_index(typeid(Transform));
    reg.registerComponent(std::move(meta));

    REQUIRE(reg.byName("Foo") != nullptr);
    REQUIRE(reg.byName("Foo")->name == "Foo");
    REQUIRE(reg.byType(std::type_index(typeid(Transform))) != nullptr);
    REQUIRE(reg.byName("Missing") == nullptr);

    // Duplicate registration is rejected (no second entry added). Install a no-op
    // assert handler so the debug EMBER_ASSERT doesn't trigger a debug-break.
    Assert::setHandler([](const char*, const char*, int, const char*) {});
    ComponentMeta dup;
    dup.name = "Foo";
    dup.type = std::type_index(typeid(Tag));
    reg.registerComponent(std::move(dup));
    Assert::resetHandler();
    REQUIRE(reg.all().size() == 1u);

    reg.clear();
}
