# Epic 09 — 3D Foundation

**Goal:** Extend the engine into 3D without breaking any existing 2D functionality. Add a Vulkan RHI backend, 3D mesh and material asset types, a perspective camera, a forward renderer for lit meshes, and 3D scene support in the editor viewport.

**Depends on:** 08-editor-v2  
**Blocks:** 10-3d-features

---

## Scope

### Vulkan RHI Backend (`engine/renderer/api/vulkan/`)
The OpenGL backend remains fully functional. Vulkan is added as an alternative selected at startup via `RHI::init(Backend::Vulkan)`.

- Instance, physical device selection, logical device, queues (graphics + transfer + compute)
- Swapchain + present queue; resize handling
- Render pass abstraction → `VulkanRenderPass`
- `VulkanVertexBuffer`, `VulkanIndexBuffer` (VMA-backed allocations)
- `VulkanUniformBuffer` — ring buffer for per-frame constants
- `VulkanShader` — SPIR-V compilation via `glslang` or `shaderc`
- `VulkanTexture2D`, `VulkanCubemap`
- `VulkanFramebuffer`
- `VulkanDescriptorSetLayout`, `VulkanDescriptorSet` — maps to RHI `ShaderResource` abstraction
- `VulkanCommandBuffer` — wraps `VkCommandBuffer`; RHI `CommandBuffer` interface implemented over it
- Validation layers enabled in Debug builds; disabled in Release
- Vulkan memory allocator: **VMA** (vendored `third_party/vma/`)

#### RHI API Extension for 3D
The existing RHI interface gains:
```cpp
RHI::drawIndexedInstanced(vbo, ibo, indexCount, instanceCount);
RHI::setDepthTest(bool enabled);
RHI::setFaceCulling(CullFace mode);     // None, Front, Back
RHI::setClearDepth(f32 depth);
RHI::bindUniformBuffer(buffer, slot);
RHI::bindDescriptorSet(set, slot);      // Vulkan path
```

### Mesh & Material Assets

#### Mesh (`engine/assets/mesh/`)
- `Mesh` — vertex buffer + index buffer on GPU; CPU-side `MeshData` (optional, can be stripped after upload)
- `MeshData` — `std::vector<Vertex3D>`, `std::vector<u32>` indices
- `Vertex3D`: `vec3 position`, `vec3 normal`, `vec3 tangent`, `vec2 uv`, `vec4 color`
- `MeshLoader` — imports `.obj` (via `tinyobjloader`, vendored) and `.gltf` (via `tinygltf`, vendored)
- Built-in primitives generated at startup: `Mesh::cube()`, `Mesh::sphere(subdivisions)`, `Mesh::plane()`
- `MeshComponent` — ECS component: `AssetHandle<Mesh>`, `AssetHandle<Material>`, cast shadow flag

#### Material (`engine/assets/material/`)
- `Material` — owns a `Shader` reference + a `std::unordered_map<string, MaterialParam>`
- `MaterialParam` — variant over `f32`, `vec2`, `vec3`, `vec4`, `bool`, `AssetHandle<Texture2D>`, `AssetHandle<Cubemap>`
- `MaterialInstance` — per-entity material override table (override only changed params; falls back to base material)
- `MaterialLoader` — loads `.ematerial` YAML files
- Initial material types:
  - `UnlitMaterial` — base color + texture, no lighting
  - `PhongMaterial` — ambient/diffuse/specular/shininess; basic Phong shading

### 3D Renderer (`engine/renderer/3d/`)

#### Forward Renderer
- `Renderer3D::beginScene(camera3D, lights)`
- `Renderer3D::submit(meshComponent, worldTransform)` — adds to opaque or transparent bucket
- `Renderer3D::endScene()` — sorts opaque front-to-back, transparent back-to-front; issues draw calls
- Opaque pass: depth write on, face culling on
- Transparent pass: depth write off, alpha blend on
- Per-frame UBO: `ViewProjectionUBO { mat4 view; mat4 proj; mat4 viewProj; vec3 cameraPos; }`
- Per-object push constant / UBO: `mat4 model; mat4 normalMatrix;`

#### Lighting
- `DirectionalLight` component: direction, color, intensity
- `PointLight` component: position (from Transform), color, intensity, range, attenuation
- Max 16 point lights per frame (simple forward, no clustered shading yet)
- Lights uploaded to a `LightUBO` each frame
- `AmbientLight` in `SceneSettings`: color + intensity

#### 3D Camera (`engine/renderer/camera/`)
- `Camera3D` component: `fovDegrees`, `near`, `far`, `isPrimary`
- `CameraSystem` updated to handle both `Camera2D` (orthographic) and `Camera3D` (perspective)
- View matrix from `Transform` (position + rotation quaternion)
- `EditorCamera3D` — orbit camera for the editor (alt + drag = orbit, scroll = dolly, middle = pan)

### Scene & ECS Updates
- `Transform` rotation field promoted from `f32` (2D Euler) to `glm::quat` — 2D usage stores a quat with rotation around Z only; backward compatible
- `WorldTransform` cached matrix upgraded to full 4×4
- `SceneSettings` gains: `AmbientLight`, `SkyColor`/`Skybox` (cubemap handle), `FogSettings`

### Editor: 3D Viewport
- `ViewportPanel` detects whether the active scene has `Camera3D` or `Camera2D` entities and activates the appropriate editor camera
- `EditorCamera3D` controls enabled when scene has 3D content
- Gizmos (ImGuizmo) already work in 3D — no changes needed
- Inspector: `Camera3D` component UI, `MeshComponent` UI, `DirectionalLight` / `PointLight` UI

### Asset Loaders
- `MeshLoader` — `.obj` via tinyobjloader; `.gltf` (static meshes only) via tinygltf
- `MaterialLoader` — `.ematerial` YAML
- `CubemapLoader` — 6-face PNG/HDR → OpenGL/Vulkan cubemap texture

---

## Exit Criteria

- [ ] A `.obj` file loads and renders as a Phong-shaded mesh in the Viewport
- [ ] A `DirectionalLight` + 4 `PointLight` entities affect the mesh shading correctly
- [ ] Perspective `Camera3D` with correct FOV; near/far clip planes verified
- [ ] `EditorCamera3D` orbits, pans, dollies without gimbal lock
- [ ] Vulkan backend renders the same Phong-shaded mesh as OpenGL (visual parity test: screenshot comparison)
- [ ] A 2D scene from Epic 07 still loads and renders correctly (regression test)
- [ ] `Transform` quaternion rotation: a 2D scene's rotation values migrate correctly

---

## Key Files Created

```
engine/renderer/api/vulkan/           (all Vulkan backend files)
engine/renderer/3d/include/ember/renderer/Renderer3D.h
engine/renderer/3d/ForwardRenderer.h/.cpp
engine/renderer/camera/Camera3D component in Components.h
engine/renderer/camera/EditorCamera3D.h/.cpp
engine/assets/mesh/Mesh.h/.cpp
engine/assets/mesh/MeshLoader.h/.cpp
engine/assets/material/Material.h/.cpp
engine/assets/material/MaterialLoader.h/.cpp
engine/assets/loaders/CubemapLoader.h/.cpp
assets/shaders/phong.vert / phong.frag
assets/shaders/unlit.vert / unlit.frag
third_party/tinyobjloader/
third_party/tinygltf/
third_party/vma/
tests/renderer/TestMeshLoading.cpp
tests/renderer/TestVulkanInit.cpp
```

---

## Notes & Decisions

- `glm::quat` is stored as XYZW in memory; the YAML serializer writes it as Euler angles (degrees) for human readability and converts on deserialize — this avoids confusing scene files
- The Vulkan backend does not need to match OpenGL feature-for-feature immediately — it needs to pass the lit-mesh render test; additional RHI features are added as needed
- Do not implement a render graph yet — the forward renderer uses a simple ordered pass list; the render graph is introduced when shadows require multi-pass rendering (Epic 10)
- `Renderer2D` and `Renderer3D` are sibling systems under `Renderer`; a scene can use both (e.g., 3D world + 2D HUD overlay drawn last, no depth test)
