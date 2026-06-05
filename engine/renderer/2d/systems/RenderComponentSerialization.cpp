#include "ember/renderer/RenderComponents.h"
#include "ember/renderer/Components2D.h"
#include "ember/renderer/SpriteAnimation.h"
#include "ember/renderer/ParticleEmitter.h"
#include "ember/renderer/UIComponents.h"
#include "ember/renderer/Tilemap.h"
#include "ember/serialization/ComponentSerialization.h"
#include "ember/ecs/ComponentRegistry.h"
#include "ember/ecs/World.h"

// This is the only renderer TU that pulls in yaml-cpp (via the serialization
// registration header). It bridges the renderer's reflected components to the
// scene serializer without the serialization module depending on the renderer.

namespace ember {

namespace {
// Tilemap holds a std::vector<u32> + nested Tileset, so it can't use the reflection
// visitors. Hand-write its scene (de)serialization (inline data block, TME-06).
ComponentMeta makeTilemapMeta() {
    ComponentMeta m;
    m.name = "Tilemap";
    m.type = std::type_index(typeid(Tilemap));

    m.serialize = [](const World& w, Entity e, void* emitterPtr) -> bool {
        if (!w.has<Tilemap>(e)) return false;
        auto& out = *static_cast<YAML::Emitter*>(emitterPtr);
        const Tilemap& tm = w.get<Tilemap>(e);
        out << YAML::Key << "Tilemap" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "width"         << YAML::Value << tm.width;
        out << YAML::Key << "height"        << YAML::Value << tm.height;
        out << YAML::Key << "tileWorldSize" << YAML::Value << tm.tileWorldSize;
        out << YAML::Key << "layer"         << YAML::Value << tm.layer;
        out << YAML::Key << "texturePath"   << YAML::Value << tm.tileset.texturePath;
        out << YAML::Key << "columns"       << YAML::Value << tm.tileset.columns;
        out << YAML::Key << "rows"          << YAML::Value << tm.tileset.rows;
        out << YAML::Key << "tiles" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (u32 t : tm.tiles) out << t;
        out << YAML::EndSeq;
        out << YAML::EndMap;
        return true;
    };

    m.deserialize = [](World& w, Entity e, const void* nodePtr,
                       const std::unordered_map<u64, Entity>&) {
        const auto& node = *static_cast<const YAML::Node*>(nodePtr);
        Tilemap tm;
        tm.width            = node["width"].as<u32>(0u);
        tm.height           = node["height"].as<u32>(0u);
        tm.tileWorldSize    = node["tileWorldSize"].as<f32>(1.0f);
        tm.layer            = node["layer"].as<i32>(0);
        tm.tileset.texturePath = node["texturePath"].as<std::string>(std::string{});
        tm.tileset.columns  = node["columns"].as<u32>(1u);
        tm.tileset.rows     = node["rows"].as<u32>(1u);
        tm.tiles.clear();
        if (auto seq = node["tiles"]) for (const auto& t : seq) tm.tiles.push_back(t.as<u32>(0u));
        tm.tiles.resize(static_cast<usize>(tm.width) * tm.height, 0u);   // guard size
        w.emplace<Tilemap>(e, std::move(tm));
    };

    return m;
}
} // namespace

void registerRenderComponents() {
    registerComponentType<SpriteRenderer>("SpriteRenderer");
    registerComponentType<Camera2D>("Camera2D");
    registerComponentType<SpriteAnimator>("SpriteAnimator");
    registerComponentType<ParticleEmitter>("ParticleEmitter");
    registerComponentType<UIImage>("UIImage");
    registerComponentType<UIText>("UIText");
    registerComponentType<UIButton>("UIButton");

    auto& reg = ComponentRegistry::instance();
    if (!reg.byName("Tilemap")) reg.registerComponent(makeTilemapMeta());
}

} // namespace ember
