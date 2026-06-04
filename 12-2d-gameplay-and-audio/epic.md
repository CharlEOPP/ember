# Epic 12 — 2D Gameplay & Audio

**Goal:** Close the gaps that stop Ember from shipping real 2D games. Add genuine audio playback, a lightweight 2D physics/collision system that fires the script collision callbacks, sprite-sheet animation, runtime tilemaps, a simple particle system, and a minimal in-game UI/HUD layer. After this epic a developer can build a complete 2D game — moving, colliding, animated, with sound and a HUD — entirely in C++ scripts on top of the existing ECS/renderer/input/asset/scripting stack.

**Depends on:** 04-renderer-2d, 05-input-and-assets, 06-scripting  
**Blocks:** — (inserted ahead of 07-editor-v1; the editor will gain panels for these systems afterwards)

> Sequencing note: this epic is being done **before** Epic 07 (Editor v1). The 3D-oriented physics (Jolt) and skeletal animation in Epic 10 do **not** cover 2D games; this epic provides the 2D-native equivalents. No new heavyweight dependency is introduced — 2D physics is hand-rolled, audio reuses the already-vendored miniaudio.

---

## Scope

### 1. Audio System (`engine/audio/`)
Turn the decode-only `AudioClip` (Epic 05/loose-ends) into real sound.

- Backend: **miniaudio** (already vendored behind `EMBER_ENABLE_AUDIO`; this epic makes audio a first-class engine module, still toggleable for headless/CI).
- `AudioEngine` — owns the miniaudio device + mixer; `init()/shutdown()`, master volume.
- Playback API (engine-facing and script-facing):
  - `AudioHandle play(AssetHandle<AudioClip>, const PlayParams&)` — `PlayParams{ volume, pitch, loop, bus }`
  - `stop(AudioHandle)`, `stopAll()`, `setVolume(AudioHandle, f32)`, `isPlaying(AudioHandle)`
  - Buses: at least **SFX** and **Music**, each with independent volume; master on top.
- `AudioClip` plays from its decoded PCM (interleaved float) via a miniaudio data source / audio buffer — no re-decoding per play.
- Script ergonomics: `Audio::play("sfx/jump.wav")` convenience (loads + plays via the AssetManager) and a `playOneShot` helper.
- `AudioListener` / positional audio is **out of scope** (2D games rarely need it); simple stereo pan parameter is optional.

### 2. 2D Physics & Collision (`engine/physics2d/`)
A lightweight, dependency-free 2D collision + dynamics system (custom AABB/circle, impulse solver).

- Components:
  - `RigidBody2D` — `bodyType` {Static, Kinematic, Dynamic}, `velocity` (vec2), `gravityScale`, `mass`, `linearDamping`, `fixedRotation`.
  - `BoxCollider2D` — half-extents (vec2), offset, `material{friction, restitution}`, `isTrigger`.
  - `CircleCollider2D` — radius, offset, material, `isTrigger`.
- `Physics2DSystem` (runs on the **fixed timestep**, integrating with `ScriptSystem`'s `onFixedUpdate` cadence):
  - Integrate velocities (gravity, damping) for dynamic bodies.
  - Broadphase: uniform spatial hash (or sweep-and-prune) to cull pairs.
  - Narrowphase: AABB↔AABB, Circle↔Circle, AABB↔Circle overlap + penetration normal/depth.
  - Resolution: impulse response + positional correction (Baumgarte) for non-trigger pairs; triggers generate events only.
  - Sync `Transform` ↔ body each step.
- **Collision events:** fire the callbacks `ScriptComponent` already declares — `onCollisionEnter/Stay/Exit(Entity)` and `onTriggerEnter/Exit(Entity)` — tracking contact pairs frame-to-frame.
- Script API on `RigidBody2D`: `applyForce`, `applyImpulse`, `setVelocity`, `getVelocity`.
- Queries: `Physics2DWorld::raycast(origin, dir, maxDist)`, `overlapBox/overlapCircle`.
- Debug draw: wire colliders via the existing `DebugDraw` (Debug builds only).

### 3. Sprite-Sheet Animation (`engine/renderer/2d/`)
The 2D analog of Epic 10's skeletal animation.

- Requires a **UV sub-rect on `SpriteRenderer`** (new field `glm::vec4 uvRect{0,0,1,1}`) so a sprite can show one frame of an atlas; `Renderer2D::drawSprite`/`drawQuad` honor it.
- `SpriteAnimationClip` asset — ordered frames (each a `TextureAtlas` region or explicit UV rect), `fps`, `loop`.
- `SpriteAnimator` component — current clip, time, `playing`, `speed`.
- `SpriteAnimationSystem` (Update phase, before render): advances each animator and writes the active frame's UV rect (and optionally texture) into its `SpriteRenderer`.
- Reuses the existing `TextureAtlas` for frame regions; clips loadable from YAML.

### 4. Tilemap Runtime (`engine/renderer/2d/`)
Runtime rendering of tile grids (editing UI is Epic 08).

- `Tileset` — a texture atlas + tile size; maps tile index → UV rect.
- `Tilemap` component — grid dimensions, tile size (world units), `std::vector<u32>` tile indices (0 = empty), layer.
- `TilemapRenderSystem` — batches visible tiles through `Renderer2D` (frustum/extent cull).
- Simple loader: tilemaps from a CSV/JSON-ish `.tilemap` file (Tiled `.tmx` import deferred).
- Optional: generate `BoxCollider2D`s for solid tiles (a "collision layer") for platformer ground.

### 5. Particles (`engine/renderer/2d/`)
- `ParticleEmitter` component — emission rate, burst, particle lifetime, start/end size, start/end color, velocity range, gravity, max particles.
- `ParticleSystem` (Update + Render) — pool-allocated particles, simulate (position/velocity/lifetime/color lerp), render as batched quads via `Renderer2D`.
- World-space and screen-space emitters.

### 6. Game UI / HUD (`engine/ui/`)
A minimal **runtime** UI layer (distinct from the editor's ImGui) drawn on `Renderer2D` in screen space.

- `UICanvas` — screen-space coordinate root; anchors (corners/center) + pixel offsets.
- Widgets: `UIText` (uses the bitmap font), `UIImage` (a sprite), `UIButton` (image + text, hover/press states driven by `Input` mouse position + click).
- Immediate-or-retained: a small retained tree updated/drawn each frame; button callbacks via `std::function`.
- Enough to build a HUD (score/health), a main menu, and a pause overlay.

### 7. Sandbox Integration
Extend the sandbox into a tiny but complete 2D game slice that exercises every system:
- A player (sprite-animated, `RigidBody2D` + `BoxCollider2D`) that runs and jumps on tile ground.
- Collectibles (trigger colliders) that play a pickup **sound** and spawn a **particle** burst.
- A **HUD** showing score; a **tilemap** level; a looping **music** track.

---

## Exit Criteria

- [ ] A sound plays on demand (`Audio::play`), loops correctly, respects per-bus and master volume, and mixes multiple simultaneous sounds without glitching.
- [ ] Two dynamic `RigidBody2D` boxes collide and separate; a body rests on static ground under gravity.
- [ ] `onCollisionEnter` fires once when two bodies first touch; `onTriggerEnter` fires for a trigger overlap without a physical response.
- [ ] `Physics2DWorld::raycast` returns the correct entity/hit point.
- [ ] A `SpriteAnimator` cycles a walk animation at the configured fps and loops seamlessly.
- [ ] A `Tilemap` renders a multi-screen level batched efficiently; solid tiles block the player.
- [ ] A `ParticleEmitter` burst renders and expires correctly; 1000+ live particles stay >60 fps.
- [ ] A `UIButton` highlights on hover and fires its callback on click; `UIText` HUD updates each frame.
- [ ] The sandbox 2D-game slice runs: animated player, tile ground collision, pickup sound + particles, HUD score, background music.
- [ ] All new modules build clean (`-Werror`), unit tests pass, `sanitize` clean; existing 2D scenes/tests unaffected (regression).

---

## Key Files Created

```
engine/audio/include/ember/audio/AudioEngine.h
engine/audio/include/ember/audio/Audio.h            # static facade
engine/audio/src/AudioEngine.cpp                    # miniaudio device + mixer
engine/physics2d/include/ember/physics2d/RigidBody2D.h
engine/physics2d/include/ember/physics2d/Colliders2D.h
engine/physics2d/include/ember/physics2d/Physics2DWorld.h
engine/physics2d/src/Physics2DSystem.cpp
engine/renderer/2d/include/ember/renderer/SpriteAnimator.h
engine/renderer/2d/src/SpriteAnimationSystem.cpp
engine/renderer/2d/include/ember/renderer/Tilemap.h
engine/renderer/2d/src/TilemapRenderSystem.cpp
engine/renderer/2d/include/ember/renderer/ParticleEmitter.h
engine/renderer/2d/src/ParticleSystem.cpp
engine/ui/include/ember/ui/UICanvas.h               # UIText/UIImage/UIButton
engine/ui/src/UISystem.cpp
sandbox/scripts/...                                 # game-slice scripts
tests/audio/TestAudioEngine.cpp
tests/physics2d/TestCollision2D.cpp
tests/physics2d/TestRigidBody2D.cpp
tests/renderer/TestSpriteAnimation.cpp
tests/renderer/TestTilemap.cpp
tests/renderer/TestParticles.cpp
tests/ui/TestUILayout.cpp
```

---

## Notes & Decisions

- **2D physics is custom, not Box2D.** A hand-rolled AABB/circle collision + impulse solver keeps the engine dependency-free and is sufficient for the vast majority of 2D games (platformers, top-down, arcade). If a project later needs joints/continuous collision, Box2D can be slotted behind the same `RigidBody2D`/collider components.
- **Audio reuses miniaudio** (already vendored). Audio becomes a normal engine module but stays compile-toggleable (`EMBER_ENABLE_AUDIO`) so headless CI and the test runner don't require an audio device. Tests use a null/loopback device or assert on mixer state, not real output.
- **Physics runs on the fixed timestep**, sharing `ScriptSystem`'s 50 Hz accumulator semantics so `onFixedUpdate` and physics stay in lockstep; collision callbacks are dispatched through the existing `ScriptComponent` virtuals (no new dispatch path).
- **Sprite animation needs a `SpriteRenderer.uvRect`** — a small, backwards-compatible addition (defaults to the full texture, so existing sprites are unchanged).
- **Runtime UI is deliberately minimal** and separate from the editor's ImGui — it ships in games; ImGui does not.
- **Tiled/`.tmx` import, audio spatialization, particle GPU instancing, and UI layout containers (flex/grid)** are explicitly deferred to keep the epic implementable.
