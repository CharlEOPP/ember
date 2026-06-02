# Epic 10 — 3D Features

**Goal:** Build the features that make 3D rendering production-quality and the engine usable for real 3D games: PBR materials, shadow maps, a physics integration, skeletal animation, and a post-processing stack.

**Depends on:** 09-3d-foundation  
**Blocks:** 11-scripting-hot-reload-and-networking

---

## Scope

### PBR Material System
Replace the Phong material from Epic 09 with a physically-based workflow.

- `PBRMaterial` parameters:
  - `albedo` (vec4 or Texture2D)
  - `metallic` (f32 or Texture2D, R channel)
  - `roughness` (f32 or Texture2D, G channel)
  - `ao` (f32 or Texture2D, B channel; packed with metallic/roughness in ORM texture)
  - `normalMap` (Texture2D, tangent space)
  - `emissive` (vec3 or Texture2D)
- Cook-Torrance BRDF: GGX NDF, Smith geometry term, Fresnel-Schlick
- Image-Based Lighting (IBL):
  - Equirectangular HDR → cubemap conversion at import time
  - Diffuse irradiance convolution (pre-computed, stored as small cubemap)
  - Specular pre-filtered env map (6 mip levels per roughness band)
  - BRDF LUT (512×512, pre-computed once at startup)
- Editor: PBR material editor UI in Inspector — sliders, color pickers, texture pickers

### Render Graph
A lightweight render graph replaces the direct pass ordering used in Epic 09. Required because shadow maps and post-processing demand multiple ordered render passes with explicit resource dependencies.

- `RenderGraph` — DAG of `RenderPass` nodes
- `RenderPass` — declares: input textures (reads), output textures (writes), execute callback
- Graph compilation: topological sort, automatic barrier insertion (Vulkan) or texture binding management (OpenGL)
- Built-in passes:
  - `ShadowMapPass`
  - `GeometryPass` (forward opaque)
  - `TransparentPass`
  - `PostProcessPass` chain
  - `UIPass` (2D overlay)
  - `BlitToSwapchainPass`

### Shadow Maps
- `DirectionalLight` shadows: single cascade shadow map (CSM with 4 cascades for large scenes, but start with 1 cascade)
- Shadow map resolution: configurable, default 2048×2048
- PCF soft shadows (3×3 kernel)
- Depth bias configurable per light component
- `PointLight` shadows: cube shadow map (6-face depth render)
- Editor: shadow frustum visualization overlay (Debug Overlays toggle)

### Jolt Physics Integration (`engine/physics/`)
[Jolt Physics](https://github.com/jrouwe/JoltPhysics) — chosen for: permissive MIT license, multi-threading, determinism, modern C++17/20.

- `PhysicsWorld` — wraps `JPH::PhysicsSystem`; fixed-timestep update via `onFixedUpdate`
- Components:
  - `RigidBody` — mass, linear/angular damping, gravity scale, is kinematic flag
  - `BoxCollider` — half-extents, offset, material (friction, restitution)
  - `SphereCollider` — radius, offset, material
  - `CapsuleCollider` — radius, half-height, offset, material
  - `MeshCollider` — convex hull or triangle mesh from `MeshAsset` (static only for triangle mesh)
- `PhysicsSystem` (FixedUpdate phase):
  - Syncs `Transform` → Jolt body before step
  - Steps `PhysicsWorld`
  - Syncs Jolt body → `Transform` after step
- Collision callbacks: Jolt contact listener → fires `ScriptComponent::onCollisionEnter/Stay/Exit`
- Queries: `PhysicsWorld::raycast(ray)`, `PhysicsWorld::overlapSphere(center, radius)`
- `RigidBody` API available from scripts: `applyForce`, `applyImpulse`, `applyTorque`, `setVelocity`, `getVelocity`
- Editor: physics shape debug draw (wire colliders) as overlay

### Skeletal Animation
- `Skeleton` asset — hierarchy of bones (name, parent index, bind-pose matrix)
- `AnimationClip` asset — per-bone keyframe tracks (position, rotation, scale); supports step, linear, cubic Hermite interpolation
- `AnimationController` component — references `Skeleton` + current `AnimationClip`; playback speed, loop, blend
- `AnimationSystem` (PreUpdate phase): samples clip at current time → builds `BoneMatrixPalette`; uploads to GPU skinning UBO
- GPU skinning shader: `layout(std140) uniform BoneMatrices { mat4 bones[128]; };` applied in vertex shader
- `.gltf` skinned mesh import via tinygltf (was static-only in Epic 09)
- Editor: animation preview in Viewport; clip timeline scrubber in Inspector

### Post-Processing Stack
- `PostProcessVolume` component (and global scene setting) — ordered list of post-process effects
- Effects implemented as fullscreen quad passes reading from the previous pass's output texture:
  - `BloomEffect` — dual-kawase blur on bright pixels; threshold + intensity settings
  - `TonemappingEffect` — ACES filmic tonemapping; exposure setting
  - `VignetteEffect` — radial darkening; intensity + radius settings
  - `FXAAEffect` — fast approximate anti-aliasing
  - `GammaCorrectionEffect` — linear → sRGB (final pass, always applied unless HDR output)
- Editor: PostProcess panel in Inspector for `PostProcessVolume`; real-time preview

---

## Exit Criteria

- [ ] PBR sphere with metallic=1/roughness=0 shows correct specular highlight under directional light
- [ ] IBL: sphere with environment map shows correct irradiance + specular reflection from HDRI
- [ ] Shadow map: directional light casts soft shadow on a floor plane; no major artifacts at default settings
- [ ] Box collider entity falls under gravity and comes to rest on a static plane collider
- [ ] `PhysicsWorld::raycast` returns correct entity under cursor (mouse-picking via ray)
- [ ] `ScriptComponent::onCollisionEnter` fires when two physics bodies first touch
- [ ] Skinned `.gltf` character plays walk animation in loop without vertex explosion
- [ ] Bloom + tonemapping chain renders correctly; toggling effects updates viewport in real time
- [ ] Full 2D scene (from Epic 07) still loads and renders at correct performance (regression)

---

## Key Files Created

```
engine/renderer/3d/PBRMaterial.h/.cpp
engine/renderer/3d/IBLPrecompute.h/.cpp
engine/renderer/graph/RenderGraph.h/.cpp
engine/renderer/graph/RenderPass.h
engine/renderer/passes/ShadowMapPass.h/.cpp
engine/renderer/passes/PostProcessPass.h/.cpp
engine/renderer/effects/BloomEffect.h/.cpp
engine/renderer/effects/TonemappingEffect.h/.cpp
engine/physics/include/ember/physics/PhysicsWorld.h
engine/physics/include/ember/physics/RigidBody.h
engine/physics/include/ember/physics/Colliders.h
engine/physics/PhysicsSystem.h/.cpp
engine/animation/Skeleton.h/.cpp
engine/animation/AnimationClip.h/.cpp
engine/animation/AnimationController.h
engine/animation/AnimationSystem.h/.cpp
assets/shaders/pbr.vert / pbr.frag
assets/shaders/shadow_depth.vert / shadow_depth.frag
assets/shaders/skinned.vert
assets/shaders/bloom_blur.frag / bloom_composite.frag
assets/shaders/tonemap.frag
third_party/JoltPhysics/
tests/physics/TestRigidBodyFall.cpp
tests/animation/TestAnimationSampling.cpp
```

---

## Notes & Decisions

- IBL pre-computation runs at asset import time, not at startup — computed cubemaps are cached as asset files so startup remains fast
- Animation blending (blend trees, state machines) is deferred — `AnimationController` supports a single active clip only; this is intentional to keep the system implementable in one epic
- Jolt runs on a background thread pool; the `PhysicsSystem` uses a double-buffer for body transforms so the render thread never blocks on physics
- The render graph implementation does not need to be optimal in this epic — correct ordering and resource tracking are the goals; GPU-side automatic barrier deduction (frame graph style) is a future refinement
