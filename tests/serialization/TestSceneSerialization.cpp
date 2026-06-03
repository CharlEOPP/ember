#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ember/scene/Scene.h"
#include "ember/serialization/YAMLSerializer.h"

#include <glm/glm.hpp>
#include <string>

using namespace ember;
using Catch::Approx;

TEST_CASE("Serialize_MultiEntityRoundtrip", "[serialization]") {
    Scene a("level");
    Entity p = a.create("Player");
    a.world().get<Transform>(p).position = {1.0f, 2.0f, 3.0f};
    a.world().get<Tag>(p).layer = "players";
    a.create("Enemy");

    const std::string yaml = YAMLSerializer::serialize(a);

    Scene b("loaded");
    auto r = YAMLDeserializer::deserialize(yaml, b);
    REQUIRE(r.has_value());

    Entity bp = b.findByName("Player");
    REQUIRE(bp != NULL_ENTITY);
    REQUIRE(b.world().get<Transform>(bp).position.x == Approx(1.0f));
    REQUIRE(b.world().get<Transform>(bp).position.z == Approx(3.0f));
    REQUIRE(b.world().get<Tag>(bp).layer == "players");
    REQUIRE(b.findByName("Enemy") != NULL_ENTITY);
}

TEST_CASE("Serialize_HierarchyRoundtrip", "[serialization]") {
    Scene a("h");
    Entity g  = a.create("GrandParent");
    Entity pa = a.create("Parent");
    Entity c  = a.create("Child");
    a.setParent(pa, g);
    a.setParent(c, pa);

    const std::string yaml = YAMLSerializer::serialize(a);

    Scene b("h2");
    REQUIRE(YAMLDeserializer::deserialize(yaml, b).has_value());

    Entity bg  = b.findByName("GrandParent");
    Entity bpa = b.findByName("Parent");
    Entity bc  = b.findByName("Child");
    REQUIRE(bg  != NULL_ENTITY);
    REQUIRE(bpa != NULL_ENTITY);
    REQUIRE(bc  != NULL_ENTITY);
    REQUIRE(b.getParent(bc)  == bpa);   // remapped parent refs intact
    REQUIRE(b.getParent(bpa) == bg);
    REQUIRE(b.getParent(bg)  == NULL_ENTITY);
}

TEST_CASE("Serialize_StableOutput", "[serialization]") {
    Scene a("s");
    a.create("A");
    a.create("B");
    REQUIRE(YAMLSerializer::serialize(a) == YAMLSerializer::serialize(a));  // deterministic
}

TEST_CASE("Serialize_UnknownComponentSkipped", "[serialization]") {
    const char* yaml = R"(version: 1
name: "t"
entities:
  - id: 1
    name: "E"
    components:
      Transform: { position: [1, 2, 3], rotation: 0, scale: [1, 1, 1] }
      Bogus: { foo: 5 }
)";
    Scene s("x");
    auto r = YAMLDeserializer::deserialize(yaml, s);
    REQUIRE(r.has_value());                       // unknown component does not abort
    Entity e = s.findByName("E");
    REQUIRE(e != NULL_ENTITY);
    REQUIRE(s.world().has<Transform>(e));
}

TEST_CASE("Serialize_PrefabRoundtrip", "[serialization]") {
    Scene a("p");
    Entity root   = a.create("Turret");
    Entity barrel = a.create("Barrel");
    a.setParent(barrel, root);

    const std::string prefab = YAMLSerializer::serializePrefab(a, root);

    Scene b("world");
    auto r = YAMLDeserializer::instantiatePrefab(b, prefab, glm::vec3{10.0f, 5.0f, 0.0f});
    REQUIRE(r.has_value());

    Entity inst = *r;
    REQUIRE(b.world().get<Transform>(inst).position.x == Approx(10.0f));
    REQUIRE(b.world().has<Children>(inst));
    REQUIRE(b.world().get<Children>(inst).children.size() == 1u);
}
