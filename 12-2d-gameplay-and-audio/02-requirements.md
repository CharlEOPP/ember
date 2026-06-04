# Epic 12 — 2D Gameplay & Audio: Requirements

**Status:** Draft  
**Epic:** 12-2d-gameplay-and-audio  
**Depends on:** 04-renderer-2d, 05-input-and-assets, 06-scripting  
**Blocks:** — (sequenced before 07-editor-v1; the editor later adds panels for these systems)

---

## 1. Overview

Provide the 2D-native gameplay systems missing from the engine so a developer can build a complete 2D game in C++ scripts: real **audio playback**, a dependency-free **2D physics/collision** system that drives the script collision callbacks, **sprite-sheet animation**, runtime **tilemaps**, a **particle** system, and a minimal runtime **UI/HUD** layer. Everything builds on the existing ECS (`World`/`Scene`), `Renderer2D`, `Input`, `AssetManager`, and `ScriptSystem`. No heavyweight third-party dependency is added — physics is hand-rolled; audio reuses the already-vendored miniaudio.

### 1.1 Grounding in the current engine

| Existing facility | How this epic uses it |
|---|---|
| `Renderer2D::drawQuad/drawSprite/drawText`, `TextureAtlas`, `DebugDraw` | tiles, particles, animated sprites, UI, collider wireframes |
| `SpriteRenderer{ AssetHandle<Texture2D>, color, layer, flipX/Y }` | gains a `uvRect` for animation frames (backwards-compatible) |
| `Scene`/`World` (entt), `ISystem`/`Phase`, `SystemScheduler` | new systems registered per phase |
| `ScriptComponent::onCollision*/onTrigger*` (declared, never fired) | **now fired** by `Physics2DSystem` |
| `ScriptSystem` fixed-timestep accumulator (50 Hz) | physics shares the same fixed cadence |
| `AssetManager` + handles + loaders + `.meta` | clips, tilesets, animation/tilemap assets |
| `AudioClip` (decoded PCM) + miniaudio (gated `EMBER_ENABLE_AUDIO`) | played by the new `AudioEngine` |
| `Input` (mouse pos/buttons, actions) | UI hit-testing / buttons |
| `Time`/`dt` via `ISystem::update` | all per-frame simulation |

### 1.2 Module / dependency layout

New static libs, all GL-free in public headers, no cycles:
- `ember_audio` → `ember_core`, `ember_assets`; PRIVATE miniaudio (when `EMBER_ENABLE_AUDIO`).
- `ember_physics2d` → `ember_core`, `ember_ecs`, `ember_scene`, `glm`; dispatches script callbacks via `ember_scripting` (PRIVATE) **or** an injected callback sink to avoid a hard cycle.
- Sprite animation, tilemap, particles live in **`ember_renderer_2d`** (they only need the renderer + ecs + assets).
- `ember_ui` → `ember_core`, `ember_renderer_2d`, `ember_input`.

---

## 2. Definitions

- **Fixed step** — the deterministic timestep (default 1/50 s) on which physics integrates and `onFixedUpdate` runs.
- **Contact** — a colliding pair this step; tracked frame-to-frame to derive enter/stay/exit.
- **Trigger** — a collider with `isTrigger = true`: generates contact events but no physical response.
- **Bus** — an audio mix group (Master → {SFX, Music}) with its own gain.
- **Voice** — one active playing instance of a clip, addressed by an `AudioHandle`.
- **Frame (animation)** — one cell of a sprite sheet, expressed as a `uvRect` (or `TextureAtlas` region).

---

## 3. Audio (`engine/audio/`)

- **AUD-01** `AudioEngine`: `init()`/`shutdown()`, owns the miniaudio device + mixer; `setMasterVolume(f32)`. Safe to run without a device (headless): init returns false and all calls no-op (NFR-04).
- **AUD-02** Buses: at least `Master`, `SFX`, `Music`; `setBusVolume(Bus, f32)`; effective gain = voice × bus × master.
- **AUD-03** `AudioHandle play(AssetHandle<AudioClip>, const PlayParams&)` where `PlayParams{ f32 volume=1, f32 pitch=1, bool loop=false, Bus bus=SFX }`. Plays from the clip's decoded PCM with no re-decode (AUD reuses `AudioClip::samples`).
- **AUD-04** Voice control: `stop(AudioHandle)`, `stopAll()`, `setVolume(AudioHandle,f32)`, `isPlaying(AudioHandle)`. Stale handles are safe no-ops.
- **AUD-05** Mixing: multiple simultaneous voices sum without clipping artifacts at reasonable counts (≥32 voices); finished non-looping voices are reclaimed automatically.
- **AUD-06** Script facade `Audio::play("sfx/jump.wav", params)` (loads via AssetManager + plays) and `Audio::playOneShot(clip)`. Null-safe if no engine.
- **AUD-07** Compile-toggle: the whole module is gated by `EMBER_ENABLE_AUDIO`; with it off, `Audio::*` and `AudioEngine` link as no-ops so games/CI build without miniaudio.
- **Out:** positional/3D audio, DSP effects, streaming from disk (clips are fully decoded). Optional stereo `pan` may be added.

## 4. 2D Physics & Collision (`engine/physics2d/`)

- **PHY-01** `RigidBody2D{ BodyType type, glm::vec2 velocity, f32 gravityScale=1, f32 mass=1, f32 linearDamping=0, bool fixedRotation=true }`; `BodyType ∈ {Static, Kinematic, Dynamic}`. Reflected/serializable.
- **PHY-02** Colliders: `BoxCollider2D{ glm::vec2 halfExtents, glm::vec2 offset, Material, bool isTrigger }` and `CircleCollider2D{ f32 radius, glm::vec2 offset, Material, bool isTrigger }`; `Material{ f32 friction, f32 restitution }`. Reflected/serializable.
- **PHY-03** `Physics2DSystem` runs on the **fixed step** (own accumulator, default 50 Hz, configurable) integrating gravity + damping for `Dynamic` bodies; `Static` never moves; `Kinematic` moves by velocity but is not pushed.
- **PHY-04** Broadphase culls candidate pairs (uniform spatial hash or sweep-and-prune); narrowphase resolves AABB↔AABB, Circle↔Circle, AABB↔Circle, producing contact normal + penetration depth.
- **PHY-05** Resolution: non-trigger pairs get impulse response (using restitution/friction) + positional correction (Baumgarte) to remove penetration; trigger pairs generate events only, no response.
- **PHY-06** `Transform` ↔ body sync each step (read `Transform.position`/scale for collider world placement before; write resolved position after). Collider extents scale with `Transform.scale` (documented).
- **PHY-07** **Collision events:** the system tracks contacts across steps and fires, on each involved entity's scripts: `onCollisionEnter` (first step touching), `onCollisionStay` (continuing), `onCollisionExit` (separated); trigger pairs fire `onTriggerEnter/Exit`. Dispatch goes through the existing `ScriptComponent` virtuals (guarded like other script calls); the physics module reaches scripts via an injected dispatch callback / `ScriptSystem` hook (no new dependency cycle).
- **PHY-08** Script API on `RigidBody2D` (usable from scripts): `applyForce(vec2)`, `applyImpulse(vec2)`, `setVelocity(vec2)`, `getVelocity()`.
- **PHY-09** Queries on `Physics2DWorld`: `raycast(origin, dir, maxDist) -> {hit, entity, point, normal, distance}`, `overlapBox`, `overlapCircle`.
- **PHY-10** Debug draw: `Physics2DSystem` draws wire colliders via `DebugDraw` in Debug builds (toggleable).
- **Out:** rotational dynamics beyond `fixedRotation=false` torque (basic only), joints, continuous collision, convex polygons (box/circle only this epic).

## 5. Sprite-Sheet Animation (`engine/renderer/2d/`)

- **SAN-01** Add `glm::vec4 uvRect{0,0,1,1}` to `SpriteRenderer` (xy = min UV, zw = max UV); default = full texture so **existing sprites are unchanged**. `Renderer2D::drawSprite`/`drawQuad` emit quad UVs from `uvRect`.
- **SAN-02** `SpriteAnimationClip` asset: ordered frames (each a `TextureAtlas` region or explicit `uvRect`), `f32 fps`, `bool loop`. Loadable from YAML via the AssetManager.
- **SAN-03** `SpriteAnimator` component: `AssetHandle<SpriteAnimationClip> clip`, `f32 time`, `f32 speed=1`, `bool playing=true`. Reflected/serializable (handle by path).
- **SAN-04** `SpriteAnimationSystem` (Update phase, before the render/sprite system): advances each playing animator by `dt*speed`, selects the current frame (wrapping if `loop`, clamping + stopping if not), and writes that frame's `uvRect` (and optional texture) into the entity's `SpriteRenderer`.
- **SAN-05** Frame selection is deterministic for a given `time`; `loop=false` clamps to the last frame and sets `playing=false`.

## 6. Tilemap Runtime (`engine/renderer/2d/`)

- **TM-01** `Tileset`: an atlas texture + tile pixel size; `uvForTile(index) -> uvRect`. (Index 0 reserved = empty.)
- **TM-02** `Tilemap` component: `u32 width,height`, `f32 tileWorldSize`, `AssetHandle<Texture2D>`/tileset ref, `std::vector<u32> tiles` (row-major), `i32 layer`.
- **TM-03** `TilemapRenderSystem` (Render phase): batches visible tiles via `Renderer2D::drawQuad` with per-tile `uvRect`; culls tiles outside the camera extent.
- **TM-04** Loader: a `.tilemap` text format (dimensions + tileset path + CSV indices) via the AssetManager. (Tiled `.tmx` deferred.)
- **TM-05** Optional collision: a flagged "solid" tile set can emit `BoxCollider2D`/static bodies for the tilemap (platformer ground), or expose `isSolid(x,y)` for script queries.

## 7. Particles (`engine/renderer/2d/`)

- **PAR-01** `ParticleEmitter` component: `emitRate` (per sec), `burstCount`, `lifetime` (min/max), `startSize`/`endSize`, `startColor`/`endColor`, `velocity` range, `gravity`, `maxParticles`, `worldSpace` (vs emitter-local), `playing`.
- **PAR-02** `ParticleSystem` (Update + Render): pool-allocated particles (no per-frame heap alloc in steady state), simulate position/velocity/age, lerp size+color over normalized life, retire dead particles, render as batched quads via `Renderer2D`.
- **PAR-03** Supports continuous emission and one-shot bursts; `≥1000` live particles sustain >60 fps (NFR-02).

## 8. Game UI / HUD (`engine/ui/`)

- **UI-01** `UICanvas`: screen-space root; widget anchors (corners/center/edges) + pixel offset; resizes with the window.
- **UI-02** Widgets: `UIText` (bitmap font + color/size), `UIImage` (sprite/atlas region + tint), `UIButton` (image + label, `Normal/Hover/Pressed` visual states).
- **UI-03** `UISystem`: each frame updates widget layout (anchor → screen rect), routes `Input` mouse position/clicks for hover/press, fires `UIButton::onClick` (`std::function<void()>`), and draws all widgets via `Renderer2D` in screen space (after the world render).
- **UI-04** Rendering is separate from the editor's ImGui and ships in games. Z-order by insertion / explicit layer.
- **Out:** fl/grid layout containers, text input fields, scroll views (later).

## 9. Sandbox Integration (`SBX`)

- **SBX-01** A 2D-game slice: a sprite-**animated** player with `RigidBody2D` + `BoxCollider2D` that runs (WASD/arrows via Input) and jumps, landing on **tilemap** ground (solid tiles → static colliders).
- **SBX-02** Collectible entities with **trigger** colliders: on `onTriggerEnter` they play a pickup **sound**, spawn a **particle** burst, increment score, and destroy themselves.
- **SBX-03** A **HUD** (`UIText` score, a `UIButton` pause) and a looping background **music** track.
- **SBX-04** Demonstrates physics debug-draw toggle and runs at >60 fps with the full scene.

## 10. Non-Functional Requirements

- **NFR-01** No new heavyweight dependency: physics/animation/tilemap/particles/UI are first-party; audio reuses miniaudio. Public headers stay free of miniaudio/GLFW/yaml leakage; GL stays behind the RHI.
- **NFR-02** Performance: 1000+ particles and a multi-screen tilemap each sustain >60 fps; physics handles a few hundred dynamic bodies at 50 Hz without frame spikes.
- **NFR-03** Determinism: physics is deterministic for a fixed step + fixed input (no per-frame `dt` in the integrator). Animation frame selection is deterministic for a given time.
- **NFR-04** Headless/CI: audio and physics run without a window/device; tests verify mixer/contact/animation state on the CPU (no real audio output, no GL).
- **NFR-05** Backwards-compatible: existing scenes, sprites, and tests are unaffected (`SpriteRenderer.uvRect` defaults to full; no behavior change without the new components).
- **NFR-06** Builds clean under `-Wall -Wextra -Wpedantic -Werror` (GCC/Clang/MSVC); `sanitize` clean (physics solver, particle pool, audio voice lifetime).

## 11. Out of Scope (this epic)

- Tiled `.tmx` import; audio spatialization / DSP / streaming; particle GPU instancing; UI layout containers & text input; physics joints / continuous collision / convex polygons; animation blend trees. (All deferred to follow-ups or the editor epics.)

## 12. Acceptance Criteria Summary

- [ ] Sounds play on demand, loop, mix (≥32 voices), and honor voice/bus/master volume; finished voices reclaim.
- [ ] Dynamic boxes collide and separate; a body rests on static ground under gravity; restitution bounces.
- [ ] `onCollisionEnter` fires once on first contact; `onTriggerEnter` fires for triggers with no physical response.
- [ ] `Physics2DWorld::raycast` returns the correct entity/point/normal.
- [ ] A `SpriteAnimator` loops a walk clip at the set fps; non-looping clips clamp + stop.
- [ ] A `Tilemap` renders a multi-screen level (culled/batched); solid tiles block the player.
- [ ] A `ParticleEmitter` burst renders and expires; 1000+ particles >60 fps.
- [ ] A `UIButton` hovers/clicks and fires its callback; `UIText` HUD updates each frame.
- [ ] Sandbox game slice runs end-to-end (animated player, tile collision, pickup sound + particles, HUD, music) at >60 fps.
- [ ] All new modules build `-Werror`; tests pass; `sanitize` clean; existing 2D scenes/tests unaffected.
