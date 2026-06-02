# Epic 06 — Scripting

**Goal:** Deliver a Unity-MonoBehaviour-style C++ scripting system that lets game logic live in clean, engine-agnostic script classes attached to entities. The scripting layer should feel like the primary game-code API, not an afterthought.

**Depends on:** 03-ecs-and-scene, 05-input-and-assets  
**Blocks:** 07-editor-v1 (Inspector must know how to display script components)

---

## Scope

### ScriptComponent Base Class (`engine/scripting/`)

```cpp
class ScriptComponent {
public:
    virtual ~ScriptComponent() = default;

    // Lifecycle — override as needed
    virtual void onStart()              {}
    virtual void onUpdate(f32 dt)       {}
    virtual void onFixedUpdate(f32 dt)  {}
    virtual void onLateUpdate(f32 dt)   {}
    virtual void onDestroy()            {}

    // Collision callbacks (wired by physics in a future epic)
    virtual void onCollisionEnter(Entity other) {}
    virtual void onCollisionStay(Entity other)  {}
    virtual void onCollisionExit(Entity other)  {}

    // Trigger callbacks
    virtual void onTriggerEnter(Entity other)   {}
    virtual void onTriggerExit(Entity other)    {}

protected:
    // Component access
    template<typename T> T&   getComponent();
    template<typename T> T*   tryGetComponent();
    template<typename T> T&   addComponent();
    template<typename T> void removeComponent();
    template<typename T> bool hasComponent() const;

    // Entity access
    Entity  getEntity()  const;
    Scene&  getScene()   const;
    World&  getWorld()   const;
    void    destroy();                        // destroy this entity next frame
    Entity  instantiate(AssetHandle<Prefab>); // spawn a prefab

    // Convenience
    const std::string& getName() const;
    void               setName(const std::string&);
    Transform&         transform();           // shorthand for getComponent<Transform>()

private:
    friend class ScriptSystem;
    Entity m_entity  = {};
    Scene* m_scene   = nullptr;
};
```

### ScriptSystem (`engine/scripting/`)
- `ScriptSystem` runs at Update phase; queries all entities with any `ScriptComponent` subclass
- Calls `onStart()` on first frame a script becomes active (deferred until after all `emplace<>` calls on that entity)
- Calls `onUpdate(dt)` every frame
- Calls `onFixedUpdate(dt)` at fixed timestep (default 50Hz, configurable)
- Calls `onLateUpdate(dt)` after all `onUpdate` calls (useful for camera follow)
- Calls `onDestroy()` before entity is destroyed; destruction is deferred to end-of-frame
- Handles exceptions: catches `std::exception` from scripts, logs error + disables script for that frame (does not crash the engine)

### Script Registration
Scripts must be registered with `ComponentRegistry` so that:
1. The serializer knows how to save/load them
2. The Inspector knows how to display them

```cpp
// In a game .cpp file (or auto-generated)
EMBER_REGISTER_SCRIPT(PlayerController)
EMBER_REGISTER_SCRIPT(EnemyAI)
```

`EMBER_REGISTER_SCRIPT` expands to a static initializer that:
- Registers the type with `ComponentRegistry`
- Provides a factory function (`new PlayerController()`)
- Associates reflected fields for serialization

### Script Serialization
- Script component fields decorated with `EMBER_FIELD` are serialized/deserialized automatically
- Fields of types `f32`, `i32`, `bool`, `std::string`, `glm::vec2/3/4`, `AssetHandle<T>` are supported natively
- Unknown field types are skipped with a warning (not a hard error)

```cpp
class PlayerController : public ScriptComponent {
    EMBER_REFLECT_SCRIPT(PlayerController)
public:
    EMBER_FIELD float speed     = 200.0f;
    EMBER_FIELD float jumpForce = 400.0f;
    EMBER_FIELD AssetHandle<AudioClip> jumpSound;

    void onStart() override { rb = tryGetComponent<RigidBody>(); }
    void onUpdate(f32 dt) override { /* ... */ }

private:
    RigidBody* rb = nullptr;  // NOT serialized — pointer, runtime only
};
```

### Script Execution Order
- Default order is registration order
- `EMBER_SCRIPT_ORDER(PlayerController, 10)` assigns explicit priority (lower = earlier)
- Within same priority, alphabetical by type name (deterministic)

### Sandbox Sample Scripts
Implement the following sample scripts to validate the system:

- `PlayerController` — WASD movement, space to jump, uses `Transform` directly (no physics yet)
- `CameraFollow` — `onLateUpdate` to lerp camera toward player entity
- `Spinner` — rotates transform by `speed * dt` each frame
- `Lifetime` — destroys entity after `duration` seconds

---

## Exit Criteria

- [ ] `PlayerController` attached to a sprite moves it with WASD at correct speed
- [ ] `CameraFollow` smoothly tracks the player across screen boundaries
- [ ] `onStart` fires exactly once, on the frame after `emplace`, not before
- [ ] `onDestroy` fires before entity is removed from the world
- [ ] Script fields serialize to YAML and round-trip correctly (PlayerController speed/jumpForce)
- [ ] Throwing an exception in `onUpdate` logs the error but does not crash; script is disabled for that entity
- [ ] 500 `Spinner` entities all rotating independently run at >60fps

---

## Key Files Created

```
engine/scripting/include/ember/scripting/ScriptComponent.h
engine/scripting/include/ember/scripting/ScriptSystem.h
engine/scripting/include/ember/scripting/ScriptRegistry.h
engine/scripting/ScriptSystem.cpp
sandbox/scripts/PlayerController.h
sandbox/scripts/CameraFollow.h
sandbox/scripts/Spinner.h
sandbox/scripts/Lifetime.h
tests/scripting/TestScriptLifecycle.cpp
tests/scripting/TestScriptSerialization.cpp
```

---

## Notes & Decisions

- Scripts are statically linked into the game binary for now — hot reload (dynamic library swap) is deferred to Epic 11
- The `EMBER_FIELD` macro on script members is optional: non-reflected fields still work at runtime, they just won't appear in the Inspector or serialize to YAML
- `onFixedUpdate` uses a simple accumulator — no interpolation between fixed steps yet (deferred to physics epic)
- The scripting system does **not** use virtual dispatch in its inner loop: it maintains a flat `std::vector<ScriptEntry>` of {entity, script*} pairs sorted by execution order, and iterates it directly
