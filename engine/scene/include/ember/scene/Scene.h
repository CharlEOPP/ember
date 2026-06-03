#pragma once
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"

#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace ember {

struct SceneSettings {
    glm::vec4 backgroundColor{0.1f, 0.1f, 0.15f, 1.0f};
};

// A named container owning a World plus its hierarchy + settings.
class Scene {
public:
    explicit Scene(std::string name) : m_name(std::move(name)) {}

    [[nodiscard]] World&             world()       { return m_world; }
    [[nodiscard]] const World&       world() const { return m_world; }
    [[nodiscard]] const std::string& name()  const { return m_name; }
    [[nodiscard]] SceneSettings&       settings()       { return m_settings; }
    [[nodiscard]] const SceneSettings& settings() const { return m_settings; }

    // Creates an entity pre-populated with Tag + Transform + WorldTransform.
    Entity create(std::string name = {});
    void   destroy(Entity e);

    Entity              findByName(std::string_view name);
    std::vector<Entity> findByTag(std::string_view layer);

    void   setParent(Entity child, Entity parent);
    Entity getParent(Entity child);

    // World-space matrix, recomputing along the dirty chain as needed.
    glm::mat4 getWorldTransform(Entity e);
    // Marks an entity (and all descendants) as needing a world-transform rebuild.
    void      markTransformDirty(Entity e);

private:
    bool isAncestor(Entity ancestor, Entity of);

    std::string   m_name;
    World         m_world;
    SceneSettings m_settings;
};

} // namespace ember
