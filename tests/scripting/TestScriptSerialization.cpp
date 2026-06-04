#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ember/scripting/RegisterScript.h"
#include "ember/scene/Scene.h"
#include "ember/serialization/YAMLSerializer.h"
#include "ember/ecs/Reflect.h"

#include <string>

using namespace ember;
using Catch::Approx;

namespace {
struct SER_Player : ScriptComponent {
    float speed     = 200.0f;
    float jumpForce = 400.0f;
    bool  canDouble = false;
};
EMBER_REFLECT(SER_Player) {
    EMBER_FIELD(speed);
    EMBER_FIELD(jumpForce);
    EMBER_FIELD(canDouble);
}

struct Reg {
    Reg() { registerScriptType<SER_Player>("SER_Player"); }   // full: ScriptRegistry + serializer
};
const Reg g_reg;
} // namespace

TEST_CASE("Script reflected fields round-trip through YAML", "[scripting]") {
    Scene a("level");
    Entity e = a.create("Hero");
    auto& sp = a.world().emplace<SER_Player>(e);
    sp.speed     = 175.5f;
    sp.jumpForce = 333.0f;
    sp.canDouble = true;

    const std::string yaml = YAMLSerializer::serialize(a);

    Scene b("loaded");
    auto r = YAMLDeserializer::deserialize(yaml, b);
    REQUIRE(r.has_value());

    Entity be = b.findByName("Hero");
    REQUIRE(be != NULL_ENTITY);
    REQUIRE(b.world().has<SER_Player>(be));
    const auto& out = b.world().get<SER_Player>(be);
    REQUIRE(out.speed == Approx(175.5f));
    REQUIRE(out.jumpForce == Approx(333.0f));
    REQUIRE(out.canDouble == true);
}
