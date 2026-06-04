#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ember/scripting/RegisterScript.h"
#include "ember/scripting/ScriptSystem.h"
#include "ember/scene/Scene.h"
#include "ember/serialization/YAMLSerializer.h"
#include "ember/serialization/AssetResolver.h"
#include "ember/assets/AssetManager.h"
#include "ember/assets/Prefab.h"
#include "ember/ecs/Reflect.h"

#include <memory>
#include <string>

using namespace ember;
using Catch::Approx;

namespace {
// A GL-free mock asset so we can exercise AssetHandle serialization without a
// real texture loader.
struct MockTex { int w = 0; };
struct MockTexLoader : IAssetLoader<MockTex> {
    std::shared_ptr<MockTex> load(const std::filesystem::path&, const AssetSettings&) override {
        return std::make_shared<MockTex>();
    }
};

struct HandleScript : ScriptComponent {
    float                 speed = 1.0f;
    AssetHandle<MockTex>  tex;
};
// Must live in the same (anonymous) namespace as the type so ADL finds it.
EMBER_REFLECT(HandleScript) {
    EMBER_FIELD(speed);
    EMBER_FIELD(tex);
}

// A prefab loader that returns a fixed YAML payload (no file needed).
struct FixedPrefabLoader : IAssetLoader<Prefab> {
    std::string yaml;
    explicit FixedPrefabLoader(std::string y) : yaml(std::move(y)) {}
    std::shared_ptr<Prefab> load(const std::filesystem::path&, const AssetSettings&) override {
        return std::make_shared<Prefab>(Prefab{yaml});
    }
};

struct Spawner : ScriptComponent {
    AssetHandle<Prefab> prefab;
    Entity              spawned = NULL_ENTITY;
    void onStart() override { spawned = instantiate(prefab); }
};

struct Reg {
    Reg() {
        registerScriptType<HandleScript>("HandleScript");        // serialized
        registerScriptTypeNoSerialize<Spawner>("Spawner_T");     // runtime only
    }
};
const Reg g_reg;
} // namespace

TEST_CASE("Script AssetHandle field round-trips by virtual path", "[scripting]") {
    AssetManager mgr;
    mgr.registerLoader<MockTex>(std::make_shared<MockTexLoader>());
    installAssetSerializationResolver(mgr);

    auto h = mgr.load<MockTex>("textures/hero.png");

    Scene a("lvl");
    Entity e = a.create("E");
    auto& s = a.world().emplace<HandleScript>(e);
    s.tex   = h;
    s.speed = 3.0f;

    const std::string yaml = YAMLSerializer::serialize(a);

    Scene b("loaded");
    REQUIRE(YAMLDeserializer::deserialize(yaml, b).has_value());

    Entity be = b.findByName("E");
    REQUIRE(be != NULL_ENTITY);
    const auto& out = b.world().get<HandleScript>(be);
    REQUIRE(out.speed == Approx(3.0f));
    REQUIRE(out.tex.valid());
    REQUIRE(mgr.pathOf(out.tex.id) == "textures/hero.png");   // resolved back to same asset
}

TEST_CASE("Script instantiate() spawns a prefab", "[scripting]") {
    // Build a one-entity prefab as YAML.
    Scene src("src");
    Entity p = src.create("Prefabbed");
    src.world().get<Transform>(p).position = {5.0f, 0.0f, 0.0f};
    const std::string prefabYaml = YAMLSerializer::serializePrefab(src, p);

    AssetManager mgr;
    mgr.registerLoader<Prefab>(std::make_shared<FixedPrefabLoader>(prefabYaml));
    auto ph = mgr.load<Prefab>("prefabs/thing.prefab");

    Scene sc("t");
    ScriptSystem sys(sc);
    sys.setAssetManager(&mgr);

    Entity e = sc.create("spawner");
    auto& s = sc.world().emplace<Spawner>(e);
    s.prefab = ph;

    sys.update(sc.world(), 0.016f);   // onStart -> instantiate()

    REQUIRE(s.spawned != NULL_ENTITY);
    REQUIRE(sc.world().valid(s.spawned));
}
