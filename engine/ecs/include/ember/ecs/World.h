#pragma once
#include "ember/ecs/Entity.h"
#include "ember/core/Types.h"
#include "ember/core/Assert.h"

#include <entt/entt.hpp>
#include <utility>

namespace ember {

class World {
public:
    World() = default;
    World(const World&)            = delete;
    World& operator=(const World&) = delete;
    World(World&&)                 = default;
    World& operator=(World&&)      = default;

    Entity create() { ++m_alive; return m_reg.create(); }

    void destroy(Entity e) {
        EMBER_ASSERT(m_reg.valid(e), "World::destroy() called on an invalid entity");
        if (m_reg.valid(e)) { m_reg.destroy(e); --m_alive; }
    }

    [[nodiscard]] bool valid(Entity e) const { return m_reg.valid(e); }
    void clear() { m_reg.clear(); m_alive = 0; }
    [[nodiscard]] usize size() const { return m_alive; }

    template<typename T, typename... Args>
    decltype(auto) emplace(Entity e, Args&&... args) {
        EMBER_ASSERT(!m_reg.all_of<T>(e), "World::emplace<T>(): component already present");
        return m_reg.emplace<T>(e, std::forward<Args>(args)...);
    }

    template<typename T> void remove(Entity e) { m_reg.remove<T>(e); }

    template<typename T> T& get(Entity e) {
        EMBER_ASSERT(m_reg.all_of<T>(e), "World::get<T>(): component absent");
        return m_reg.get<T>(e);
    }
    template<typename T> const T& get(Entity e) const {
        EMBER_ASSERT(m_reg.all_of<T>(e), "World::get<T>(): component absent");
        return m_reg.get<T>(e);
    }

    template<typename T> [[nodiscard]] T* tryGet(Entity e) { return m_reg.try_get<T>(e); }
    template<typename T> [[nodiscard]] const T* tryGet(Entity e) const { return m_reg.try_get<T>(e); }

    template<typename... T> [[nodiscard]] bool has(Entity e) const { return m_reg.all_of<T...>(e); }

    template<typename... T> [[nodiscard]] auto view() { return m_reg.view<T...>().each(); }

    template<typename T> auto on_construct() { return m_reg.on_construct<T>(); }
    template<typename T> auto on_destroy()   { return m_reg.on_destroy<T>();   }
    template<typename T> auto on_update()    { return m_reg.on_update<T>();     }

    [[nodiscard]] entt::registry&       registry()       { return m_reg; }
    [[nodiscard]] const entt::registry& registry() const { return m_reg; }

private:
    entt::registry m_reg;
    usize          m_alive = 0;
};

} // namespace ember
