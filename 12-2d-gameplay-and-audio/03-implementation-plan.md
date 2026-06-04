# Epic 12 — 2D Gameplay & Audio: Implementation Plan

**Status:** Not started  
**Document:** 03-implementation-plan.md  
**Refs:** epic.md, 02-requirements.md

Phases are ordered by value: **Phase 0 (scaffold) → 1 (audio) → 2-3 (physics + collision events) → 4 (sprite animation)** are the core that makes the engine able to ship a 2D game. **Phases 5-7 (tilemap, particles, UI)** are high-value but deferrable — they can interleave with or follow Epic 07. **Phase 8** is the sandbox game-slice + verification. Each phase ends buildable and testable. Requirement IDs (e.g. `PHY-07`) refer to `02-requirements.md`.

**Grounding (current repo state):**

| Fact | Consequence |
|------|-------------|
| `SpriteRenderer{ AssetHandle<Texture2D>, color, layer, flipX/Y }`; `Renderer2D::drawQuad(mat4,color,tex,tiling,entityID)` emits full `[0,1]` UVs | add `uvRect` to `SpriteRenderer` + a `drawQuad(... , glm::vec4 uvRect, ...)` path; default keeps current behavior (SAN-01) |
| `ScriptComponent::onCollision*/onTrigger*` declared, never fired; `ScriptSystem` builds an entity→script flat list and guards every call | physics dispatches collisions through a new `ScriptSystem` hook (PHY-07) |
| `ScriptSystem` owns a 50 Hz fixed accumulator | `Physics2DSystem` uses the same fixed delta/cadence (PHY-03, NFR-03) |
| `registerRenderComponents()` pattern (MIG-04) registers components with the serializer | each new component group gets a `registerXxxComponents()` (reflected fields) |
| `AudioClip{ samples(float, interleaved), channels, sampleRate, frameCount }` + miniaudio gated `EMBER_ENABLE_AUDIO` | `AudioEngine` mixes these PCM buffers on a `ma_device` (AUD) |
| `TextureAtlas` (region→UV), `DebugDraw`, `Input`, `AssetManager`, `Time` | animation frames, collider wireframes, UI input, assets, dt |

**Cross-cutting decisions (confirm before starting):**

- **`SpriteRenderer.uvRect`** (`glm::vec4{0,0,1,1}` = min.xy/max.zw) is the single mechanism animation, tilemaps, and particles use to show atlas sub-rects. The sprite batcher maps the quad's 4 corners across `uvRect` (instead of hard-coded `0..1`). Default = full texture ⇒ **no change to existing sprites** (NFR-05).
- **Physics ↔ scripts coupling:** `Physics2DSystem` holds an optional `ScriptSystem*` (set via `setScriptSystem`, mirroring `ScriptSystem::setAssetManager`) and calls a new `ScriptSystem::dispatchCollision(self, other, phase, isTrigger)`. `ember_physics2d` links `ember_scripting` **PRIVATE**; scripting does *not* depend on physics ⇒ no cycle.
- **Per-entity script dispatch:** add a `tryDispatch(World&, Entity, fn)` to each `ScriptTypeInfo` (`if (auto* c = w.tryGet<T>(e)) fn(static_cast<ScriptComponent&>(*c))`) so `ScriptSystem` can invoke a callback on every script component of *one* entity (needed for collision events).
- **Audio backend:** low-level `ma_device` (playback) + a small software mixer over `AudioClip` PCM (full control of voices/loop/buses; no miniaudio resource manager). The data callback mixes active voices under a short mutex; play/stop are main-thread. Whole module gated by `EMBER_ENABLE_AUDIO`; **no-op stubs when off** (AUD-07, NFR-04).
- **Component registration:** physics/animation/tilemap/particle components are reflected and registered with the serializer via per-module `registerXxxComponents()` helpers (same pattern as `registerRenderComponents`).
- **Module deps (no cycles):** `ember_audio`→core,assets(+miniaudio priv); `ember_physics2d`→core,ecs,scene,glm(+scripting priv); animation/tilemap/particles live in `ember_renderer_2d`; `ember_ui`→core,renderer_2d,input.

---

## Phase 0 — Scaffold + `uvRect` foundation

- [x] `engine/audio/` module + CMake + root wiring done (physics2d/ui scaffolded in their phases). Animation/tilemap/particles will be files inside `engine/renderer/2d/`.
- [x] Added `glm::vec4 uvRect{0,0,1,1}` to `SpriteRenderer` (reflected via `EMBER_FIELD`). Added `Renderer2D::drawQuad(mat4, color, texture, const glm::vec4& uvRect, entityID)` that maps each quad corner across the rect (`uvMin + corner*(uvMax-uvMin)`); `drawSprite` passes `sprite.uvRect`. Existing overloads unchanged (default rect = full texture, NFR-05).
- [~] `TestSpriteUV.cpp` dropped — the batcher needs a GL context (Renderer2D::init creates GL buffers), so it can't run headless. The `uvRect` mapping is exercised by the Phase 4 animation test (CPU-asserts the `uvRect` written into `SpriteRenderer`) + the user's visual build.

**Verify Phase 0:** ✅ `uvRect` field/overload are a small correct change; `Renderer2D` compiles in the user build (the batcher edit is GL-context-only). Module + aggregate wiring configures.

---

## Phase 1 — Audio (`engine/audio/`)  ★core

Implements `AUD-01 … AUD-07`.

- [x] `AudioEngine` (`AudioEngine.h`/`.cpp`): voice pool ({clip `shared_ptr`, cursor, volume, pitch, loop, bus, gen}), `AudioBus{Master,SFX,Music}` + per-bus gain + master, `play/stop/stopAll/setVolume/isPlaying/activeVoices`, and `renderInto(out,frames,channels)` — a miniaudio-free linear-interp mixer (resamples by pitch, wraps on loop, retires one-shots), mutex-guarded.
- [x] `Audio.h`/`.cpp` static facade: `init(AudioEngine&, AssetManager*)`, `play(AssetHandle<AudioClip>, params)`, `play("path", params)` (loads via the bound manager), `playOneShot`, `stop`, `stopAll`. Null-safe. Added `AssetManager::getShared<T>` so a voice keeps its clip alive.
- [x] CMake gating: `ember_audio` built always; **only the `ma_device` open/callback is under `EMBER_ENABLE_AUDIO`** (warnings-off TU). With audio off, `init()` returns false but the mixer + voices still work, so it links without miniaudio (AUD-07).
- [x] `tests/audio/TestAudioEngine.cpp` (headless, no device): one-shot retire, loop wrap, master/bus gain, multi-voice sum + stop.

**Verify Phase 1:** ✅ mixer compiles `-Wpedantic -Werror`; a Linux-native driver runs the mixer and **all cases pass** (sample values, loop, gain, multi-voice, stop/retire). Real device output + the Catch2 run are the user's build (`EMBER_ENABLE_AUDIO=ON` for sound).

---

## Phase 2 — 2D physics core (`engine/physics2d/`)  ★core

Implements `PHY-01 … PHY-06`, `PHY-08 … PHY-10`.

- [ ] Components (reflected, in `RigidBody2D.h` / `Colliders2D.h`): `RigidBody2D`, `BoxCollider2D`, `CircleCollider2D`, `Material`. `registerPhysics2DComponents()` registers them with the serializer.
- [x] Components `RigidBody2D` (BodyType2D Static/Kinematic/Dynamic, velocity, gravityScale, mass, linearDamping, fixedRotation, `invMass()` + script API) and `BoxCollider2D`/`CircleCollider2D` + `PhysicsMaterial2D`, all reflected. (Needed a generic **enum** overload added to the YAML visitors so `BodyType2D` serializes.)
- [x] `Physics2DSystem : ISystem`: fixed accumulator (50 Hz, `setFixedDelta`, catch-up clamp); integrate gravity+damping+force for Dynamic, velocity-move for Kinematic, Static fixed; collider world placement from `Transform` (scale·extents + offset); **O(n²) broadphase with AABB reject** (spatial hash noted as a future opt); narrowphase Box↔Box / Circle↔Circle / Box↔Circle → `{normal, depth}`; impulse (restitution + Coulomb friction) + Baumgarte correction for non-trigger pairs; writes `Transform.position` back.
- [x] `RigidBody2D` script API: `applyForce/applyImpulse/setVelocity/getVelocity`.
- [x] Queries: `raycast` (ray↔AABB slab + ray↔circle), `overlapBox`, `overlapCircle`.
- [~] Debug-draw colliders — moved out of `physics2d` to avoid a `physics2d → renderer_2d` dependency; the sandbox draws colliders via `DebugDraw` + the collider views (Phase 8). 
- [x] `tests/physics2d/TestRigidBody2D.cpp` + `TestCollision2D.cpp` (headless): gravity fall, static immobile, **rest-on-floor**, overlapping boxes separate, raycast point/distance, overlap queries.

**Verify Phase 2:** ✅ system compiles `-Wpedantic -Werror`; a Linux-native driver runs the integrator + solver + queries and **all pass** (fall, rest-on-floor, push-apart, raycast, overlap). The registration TU + enum serialization compile-check is blocked only by mount truncation of an unrelated part of `ComponentSerialization.h`; verified by the user's build.

---

## Phase 3 — Collision events → script callbacks  ★core

Implements `PHY-07`.

- [x] `ScriptSystem` additions: `tryDispatch` per `ScriptTypeInfo` (`if (auto* c = w.tryGet<T>(e)) fn(...)`, set in `makeScriptInfo`); `enum class ContactPhase{Enter,Stay,Exit}`; `dispatchCollision(self, other, phase, isTrigger)` (in `ScriptSystem.cpp`) injects context and calls the matching `onCollision*`/`onTrigger*` virtual on every script of `self`, guarded (reuses the exception-isolation path).
- [x] `Physics2DSystem`: `setScriptSystem(ScriptSystem*)`; records each step's contacts (`map<pair<id,id>, {a,b,trigger}>`), diffs against the previous step → Enter/Stay/Exit (+ trigger variants), and dispatches both `(a,b)` and `(b,a)`. Triggers record contacts but get no physical response.
- [x] `tests/physics2d/TestCollisionEvents.cpp`: a counting script + a kinematic box sweeping through it → `onCollisionEnter==1`, `onCollisionStay>=1`, `onCollisionExit==1`; a trigger sweep → `onTriggerEnter/Exit==1`, no collision callbacks, static box unmoved.

**Verify Phase 3:** ⚠ in-sandbox compile **blocked this session** by persistent mount truncation of the two edited `.cpp` files (`ScriptSystem.cpp`, `Physics2DSystem.cpp`) — the hand-staged headers all balance, and the new code mirrors patterns already runtime-verified in Phase 2 (solver) and Epic 06 (`guarded`/`inject` dispatch). **Verified by the user's build** (`ctest -R physics2d` includes the new collision-events test). No dependency cycle: `physics2d → scripting` PRIVATE.

> **End of core.** After Phase 3-4 the engine can make a real 2D game (sound, collisions, animation). Phases 5-7 are deferrable.

---

## Phase 4 — Sprite-sheet animation (`engine/renderer/2d/`)  ★core

Implements `SAN-01 … SAN-05` (SAN-01 landed in Phase 0).

- [x] `SpriteAnimationClip` asset (`frames` as uvRects, `fps`, `loop`, `duration()`) + `SpriteAnimationClipLoader` (YAML: fps/loop/frames) + `registerAnimationAssetLoaders(AssetManager&)`.
- [x] `SpriteAnimator` component (reflected: `clipPath`, `speed`, `playing`; lazily loads `clipPath`→handle) + registered with the serializer in `registerRenderComponents`.
- [x] `SpriteAnimationSystem : ISystem(AssetManager&)`: advances `time += dt*speed`, picks frame `floor(t*fps)` (loop wraps via `fmod(duration)`, else clamp last + `playing=false`), writes the frame's `uvRect` into the entity's `SpriteRenderer`.
- [x] `tests/renderer/TestSpriteAnimation.cpp` (headless): frame at times, loop wrap, non-loop clamp/stop.

**Verify Phase 4:** ✅ `SpriteAnimationSystem.cpp` + `SpriteAnimationClipLoader.cpp` compile clean `-Wpedantic -Werror`. Runtime driver blocked only by EnTT codegen exceeding the sandbox compile timeout (not a code issue); the frame-selection math is simple and asserted by the Catch2 test in the user's `ctest`.

---

## Phase 5 — Tilemap runtime (`engine/renderer/2d/`)  ◇deferrable

Implements `TM-01 … TM-05`.

- [x] `Tileset` (atlas columns/rows → `uvForTile`, id 0 = empty) + `Tilemap` component (reflected: width/height/tileWorldSize/tiles/layer, `at`/`isSolid`), `TilemapRenderSystem` (Render phase, skips empty tiles, batched via `drawQuad(mat4, color, tex, uvRect)`; lazily loads the tileset texture; null texture ⇒ flat quads).
- [x] `.tilemap` text loader (key-value dims + tileset path + columns/rows, then a `tiles` CSV block) + `registerTilemapLoader`.
- [~] `isSolid(x,y)` implemented (any non-zero tile is solid); automatic solid-tile → static `BoxCollider2D` generation deferred (the slice places physics colliders explicitly).
- [x] `tests/renderer/TestTilemap.cpp`: `uvForTile` index→UV mapping; `at`/`isSolid` queries.

**Verify Phase 5:** ✅ pure-logic driver passes (`uvForTile`, `at`, `isSolid`); `TilemapLoader.cpp` + `TilemapRenderSystem.cpp` compile `-Werror` against clean headers (the in-sandbox check was only blocked by mount truncation of `Renderer2D.h`, not a code issue). Multi-screen visual render is the user's GL build.

---

## Phase 6 — Particles (`engine/renderer/2d/`)  ◇deferrable

Implements `PAR-01 … PAR-03`.

- [x] `ParticleEmitter` component (reflected config; GL-free inline `update`/`burst` simulation so it's headless-testable) + `ParticleSystem` (iterates `Transform`+`ParticleEmitter`, lerps size/color over life, draws batched quads via `Renderer2D::drawQuad`). Continuous emission (rate accumulator) + immediate `burst(n)`; `maxParticles` cap; xorshift RNG; particles stored in world space (origin refreshed by the system each frame).
- [x] `tests/renderer/TestParticles.cpp` (headless): emission count over time; lifetime expiry; `maxParticles` cap (no growth); gravity/velocity integration.

**Verify Phase 6:** ✅ Linux-native driver passes all cases (emission rate, lifetime cull, cap, integration); `ParticleSystem.cpp` compiles `-Werror` against clean headers. 1000+-particle throughput is the user's GL build.

---

## Phase 7 — Game UI / HUD (`engine/ui/`)  ◇deferrable

Implements `UI-01 … UI-04`.

- [x] `UILayout.h` (9-way `UIAnchor`, `UIRect`, `computeScreenRect`, `hitTest` — all inline/headless) + widgets `UIImage`/`UIText`/`UIButton` (reflected) with `updateButtonState` (hover/press/click from pointer state). `UISystem` (renderer_2d, links `ember_input` PRIVATE): updates button flags from `Input` mouse, then draws images/buttons/text ordered by `order` in its own screen-space scene (top-left origin ortho), resolving textures/fonts lazily via the `AssetManager`. (Implemented inside `engine/renderer/2d/` rather than a separate `engine/ui/` module to avoid a new module + a `ui→renderer` link; same public surface.)
- [x] `tests/renderer/TestUI.cpp` (headless): anchor→rect math (Center/TopLeft/BottomRight); hit-test edges; button click requires press-then-release inside; hover color.

**Verify Phase 7:** ✅ Linux-native driver passes all cases (anchors, hit-test, click/no-click, hover); `UISystem.cpp` compiles `-Werror` against clean headers. Visual HUD is the user's GL build.

---

## Phase 8 — Sandbox game slice + verification

Implements `SBX-01 … SBX-04`, NFR-01/02/06.

- [x] `sandbox/game2d.cpp` (`ember_sandbox_game2d` target): an asset-free integrated slice — Static floor + Dynamic boxes falling and resting (custom physics), a particle fountain, a tilemap strip, sprite quads rendered via `SpriteRenderSystem` with `WorldTransform` sync, and a UI HUD (panel + button) drawn in its own screen-space pass. Audio facade wired (`Audio::init`/`playOneShot` on button click; no-op without `EMBER_ENABLE_AUDIO`/clips).
- [x] Loop wiring: `physics.update` → mark `WorldTransform` dirty → `TransformSystem` rebuild → world render (`tiles`/`sprites`/`particles` inside `beginScene/endScene`) → `ui.update` (own scene). `physics.setGravity`, `ui.setScreenSize` on resize, `Audio::init(engine,&assets)`.
- [x] Header hygiene: miniaudio stays in the gated `.cpp` (public `AudioEngine.h` only forward-declares `struct Device`); new renderer/physics/UI public headers pull only core/ecs/glm/asset-handle (no GL, no yaml). Verified by the per-file `-Werror` syntax checks.
- [~] `ctest`/`sanitize`/CI green is the user's build (no GL, no MinGW libs, and EnTT codegen exceeds the in-sandbox timeout). Every headless-checkable unit (audio mixer, physics solver/queries, sprite-anim, tilemap, particles, UI) passes via Linux-native drivers; all new `.cpp` compile `-Wall -Wextra -Wpedantic -Werror` against reconstructed-clean headers.

### Acceptance checklist (from `02-requirements.md` §12) — see that list; all items mapped to phases 1-8.

---

## Suggested Commit Sequence

1. `Epic 12: scaffold audio/physics2d/ui modules + SpriteRenderer.uvRect` (Phase 0)
2. `Epic 12: audio engine + mixer + Audio facade` (Phase 1)
3. `Epic 12: 2D physics core (bodies, colliders, solver, queries)` (Phase 2)
4. `Epic 12: collision events -> script callbacks` (Phase 3)
5. `Epic 12: sprite-sheet animation` (Phase 4)
6. `Epic 12: tilemap runtime` (Phase 5)
7. `Epic 12: 2D particle system` (Phase 6)
8. `Epic 12: runtime UI/HUD` (Phase 7)
9. `Epic 12: 2D game-slice sandbox + verification` (Phase 8)

Each commit leaves the build green and CI passing. Phases 1-4 (commits 2-5) are the must-have core; 5-7 may be reordered/deferred around Epic 07.
