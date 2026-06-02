# Epic 04 — 2D Renderer

**Goal:** Build the full 2D rendering pipeline — texture loading, sprite batching, camera, text, and debug draw. At the end of this epic the sandbox can render a scene with hundreds of sprites at 60fps.

**Depends on:** 02-foundation, 03-ecs-and-scene  
**Blocks:** 07-editor-v1

---

## Scope

### RHI Completion (`engine/renderer/api/`)
- Complete the OpenGL backend beyond the stub from Epic 02:
  - `OpenGLVertexBuffer` — dynamic and static, mapped for batching
  - `OpenGLIndexBuffer`
  - `OpenGLVertexArray`
  - `OpenGLShader` — GLSL compilation, uniform upload (vec2/3/4, mat3/4, int, float, sampler array)
  - `OpenGLTexture2D` — load from STB, mip generation, filtering modes, wrap modes
  - `OpenGLFramebuffer` — color + depth attachments, resize
  - `OpenGLRenderbuffer`
- `ShaderLibrary` — cache compiled shaders by name; reload from disk in debug builds

### Texture Asset Loader
- `Texture2DLoader` implementing `IAssetLoader<Texture2D>`
- Supports PNG, JPG, BMP via stb_image
- `TextureSpec`: width, height, format (RGBA8, RGB8), filter (Linear, Nearest), wrap (Repeat, Clamp)
- Texture atlas support: `TextureAtlas` slices a single texture into named sub-regions

### Renderer2D (`engine/renderer/2d/`)

#### Sprite Batcher
- `Renderer2D::beginScene(camera)` — uploads view/projection UBO
- `Renderer2D::drawSprite(transform, spriteComponent)`
- `Renderer2D::drawQuad(position, size, color)`
- `Renderer2D::drawQuad(position, size, texture, tintColor, tilingFactor)`
- `Renderer2D::endScene()` — flushes all batches
- Vertex layout: `[vec3 pos, vec4 color, vec2 uv, float texIndex, float entityID]`
- Batch limits: 4096 quads, 32 texture slots per batch; auto-flush on overflow
- `RendererStats`: draw calls, quad count, vertex count — exposed to debug overlay

#### Shader Assets
- `sprite.glsl` — textured quad with tint
- `flat_color.glsl` — untextured quad
- `text.glsl` — SDF or bitmap font rendering

#### Components
```
SpriteRenderer   — AssetHandle<Texture2D>, color (vec4), layer (int), flipX, flipY
Camera2D         — orthographic size, near/far, isPrimary
```

### Camera System (`engine/renderer/camera/`)
- `CameraSystem` (Update phase) — queries entities with `Camera2D` + `Transform`; computes view matrix from transform, projection matrix from `Camera2D.size`; uploads to a `CameraUBO`
- `EditorCamera` — free-pan/zoom orthographic camera used by the editor viewport; not an ECS entity

### Text Rendering
- `FontAsset` — TTF loaded via stb_truetype; generates bitmap atlas at specified sizes
- `Renderer2D::drawText(text, font, position, size, color)`
- UTF-8 aware, left-to-right only (RTL / complex shaping deferred)

### Debug Draw
- `DebugDraw::line(a, b, color)`
- `DebugDraw::rect(position, size, color)`
- `DebugDraw::circle(center, radius, color, segments=32)`
- `DebugDraw::flush()` — called at end of frame, compiled out in Release
- Implemented as a separate immediate-mode line batcher

### Tilemap Rendering
- `TilemapComponent` — references a `TilemapAsset` (grid of tile IDs + tileset texture atlas)
- `TilemapRenderer` system — builds a static vertex buffer on load, re-uploads on dirty; frustum-culls tiles outside camera view

---

## Exit Criteria

- [ ] 1000 sprites with 8 different textures render at >60fps on integrated GPU (batching verified via RendererStats — should be ≤8 draw calls)
- [ ] `Camera2D` ortho camera correctly transforms world → screen; zoom and pan work
- [ ] Text renders a Latin character set correctly from a TTF file
- [ ] Debug draw lines and rects appear in sandbox; disappear in Release build
- [ ] Tilemap with 32×32 grid renders correctly with a 16px tileset
- [ ] Framebuffer render target works (render scene to texture, display as quad)

---

## Key Files Created

```
engine/renderer/api/opengl/OpenGLVertexBuffer.h/.cpp
engine/renderer/api/opengl/OpenGLShader.h/.cpp
engine/renderer/api/opengl/OpenGLTexture2D.h/.cpp
engine/renderer/api/opengl/OpenGLFramebuffer.h/.cpp
engine/renderer/2d/include/ember/renderer/Renderer2D.h
engine/renderer/2d/SpriteBatcher.h/.cpp
engine/renderer/2d/DebugDraw.h/.cpp
engine/renderer/camera/CameraSystem.h/.cpp
engine/renderer/camera/EditorCamera.h/.cpp
engine/assets/loaders/Texture2DLoader.h/.cpp
engine/assets/loaders/FontLoader.h/.cpp
assets/shaders/sprite.glsl
assets/shaders/flat_color.glsl
assets/shaders/text.glsl
tests/renderer/TestSpriteBatch.cpp
sandbox/SpriteStressTest.cpp
```

---

## Notes & Decisions

- The `entityID` field in the vertex layout is included now (even though picking isn't used yet) — it costs nothing and enables editor entity picking in Epic 07 without re-architecting the batch
- Layer sorting is integer-based; within the same layer, draw order is submission order — no painter's algorithm needed for opaque 2D sprites
- Transparent sprites (alpha < 1) are placed in a separate back-to-front pass automatically based on `SpriteRenderer.color.a`
- `TilemapRenderer` is a stretch goal for this epic; can move to Epic 08 if behind schedule
