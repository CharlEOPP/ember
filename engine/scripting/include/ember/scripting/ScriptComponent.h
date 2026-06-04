#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"
#include "ember/scene/Scene.h"
#include "ember/assets/AssetHandle.h"

#include <string>

namespace ember {

class ScriptSystem;
struct Prefab;   // asset type TBD — see 02-requirements.md §11 / SC-08

// Base class for C++ game logic, Unity-MonoBehaviour style. Subclass it, override
// the lifecycle hooks, and attach it to an entity as an ordinary component
// (world.emplace<MyScript>(e)). ScriptSystem drives the callbacks and injects the
// owning entity/scene before onStart. Helpers resolve through the owning entity.
class ScriptComponent {
public:
    virtual ~ScriptComponent() = default;

    // ---- Lifecycle (override as needed) ----
    virtual void onStart()             {}
    virtual void onUpdate(f32 /*dt*/)      {}
    virtual void onFixedUpdate(f32 /*dt*/) {}
    virtual void onLateUpdate(f32 /*dt*/)  {}
    virtual void onDestroy()           {}

    // ---- Physics callbacks: declared so script code compiles; invoked by a
    // future physics epic (SC-02), not by this one. ----
    virtual void onCollisionEnter(Entity /*other*/) {}
    virtual void onCollisionStay(Entity /*other*/)  {}
    virtual void onCollisionExit(Entity /*other*/)  {}
    virtual void onTriggerEnter(Entity /*other*/)   {}
    virtual void onTriggerExit(Entity /*other*/)    {}

protected:
    // ---- Component access (on the owning entity) ----
    template<typename T> T&   getComponent()       { return world().get<T>(m_entity); }
    template<typename T> T*   tryGetComponent()    { return world().tryGet<T>(m_entity); }
    template<typename T> T&   addComponent()       { return world().emplace<T>(m_entity); }
    template<typename T> void removeComponent()    { world().remove<T>(m_entity); }
    template<typename T> bool hasComponent() const { return world().has<T>(m_entity); }

    // ---- Entity / scene access ----
    Entity getEntity() const { return m_entity; }
    Scene& getScene()  const { return *m_scene; }
    World& getWorld()  const { return m_scene->world(); }
    Transform& transform()   { return getComponent<Transform>(); }

    const std::string& getName() const;          // via the entity's Tag
    void               setName(const std::string& name);
    void               destroy();                // deferred to end of frame
    Entity             instantiate(AssetHandle<Prefab> prefab);   // SC-08 (stubbed)

    bool isEnabled() const { return m_enabled; }

private:
    friend class ScriptSystem;
    World& world() const { return m_scene->world(); }

    Entity        m_entity{NULL_ENTITY};
    Scene*        m_scene   = nullptr;
    ScriptSystem* m_system  = nullptr;
    bool          m_started = false;   // onStart has fired
    bool          m_enabled = true;    // false after a thrown exception (SS-08)
};

} // namespace ember
