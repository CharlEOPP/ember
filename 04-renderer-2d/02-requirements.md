# Epic 04 — 2D Renderer: Requirements

**Status:** Draft  
**Epic:** 04-renderer-2d  
**Depends on:** 02-foundation, 03-ecs-and-scene, 01-vision.md §11  
**Blocks:** 07-editor-v1

---

## 1. Overview

This document specifies the complete requirements for the 2D Renderer epic. It turns the engine from "data model only" into something that draws. It delivers:

1. A **completed RHI OpenGL backend**: dynamic vertex buffers, textures, framebuffers, a full shader abstraction, and a shader cache.
2. **Texture loading** from image files via stb_image, plus texture atlases.
3. A **`Renderer2D`** with a batched sprite/quad renderer driven by ECS `Transform` + `SpriteRenderer`.
4. A **camera system** (`Camera2D` orthographic) plus a free-look `EditorCamera`.
5. **Text rendering** from TTF fonts (stb_truetype).
6. **Debug draw** (lines/rects/circles), compiled out in Release.
7. A **tilemap renderer** (stretch goal).

The measure of success: the sandbox renders a scene of hundreds of textured sprites — positioned by the ECS hierarchy from Epic 03 — at ≥60fps in a handful of draw calls.

### 1.1 Scoping note — asset system dependency

The full handle-based `AssetManager`/`AssetHandle<T>` system is **Epic 05**. To avoid a forward dependency, this epic uses a lightweight texture path: loaders return `std::shared_ptr<ITexture2D>`, and `SpriteRenderer` holds a `std::shared_ptr<ITexture2D>` (a "texture reference"). Epic 05 will migrate these to `AssetHandle<Texture2D>` behind the same `SpriteRenderer` field — a localized change. Requirements below are written against the shared_ptr form; the migration is explicitly out of scope here.

---

## 2. Definitions

| Term | Meaning |
|------|---------|
| **RHI** | Render Hardware Interface — the backend-agnostic GPU abstraction (from Epic 02) |
| **Batch** | A group of quads accumulated into one buffer and submitted in one draw call |
| **Quad** | Two triangles forming a textured/colored rectangle — the unit of 2D drawing |
| **Texture slot** | A bound texture unit referenced by a per-vertex `texIndex` within a batch |
| **Atlas** | A single texture subdivided into named sub-regions (UV rectangles) |
| **NDC / world / screen** | Normalized-device, world, and pixel coordinate spaces |
| **UBO** | Uniform Buffer Object (used for the camera view/projection) |
| **SDF** | Signed-distance-field (a text-rendering technique; bitmap is the alternative) |

---

## 3. RHI Completion Requirements (`engine/renderer/api/`)

Builds on the Epic 02 stub (`IVertexBuffer`, `IIndexBuffer`, `IShader`, `ITexture2D`, `IFramebuffer` interfaces already exist).

### 3.1 Buffers

| ID | Requirement |
|----|-------------|
| RHI-01 | `OpenGLVertexBuffer` SHALL support both **static** (data at construction) and **dynamic** (`BufferUsage::Dynamic`, empty allocation + later `setData`) creation. |
| RHI-02 | A dynamic vertex buffer SHALL support partial updates via `setData(const void* data, u32 sizeBytes)` without reallocating the GPU buffer when the new size ≤ capacity. |
| RHI-03 | The vertex buffer SHALL carry a **layout** describing its attributes (type, count, normalized, offset, stride) so the VAO can be configured generically — no hard-coded attribute setup. |
| RHI-04 | `OpenGLVertexArray` SHALL bind a vertex buffer + its layout and an index buffer, and SHALL be the unit bound before a draw call. |
| RHI-05 | `OpenGLIndexBuffer` SHALL accept a `u32` index array and report its count. |

### 3.2 Shaders & Shader Library

| ID | Requirement |
|----|-------------|
| RHI-06 | `OpenGLShader` SHALL compile a vertex + fragment program from GLSL source, log compile/link errors via `EMBER_LOG_ERROR` with the info log, and report validity. |
| RHI-07 | Shaders SHALL be loadable from a single `.glsl` file containing both stages, separated by `#type vertex` / `#type fragment` directives. |
| RHI-08 | `IShader` uniform setters SHALL cover: `int`, `int[]` (sampler arrays), `float`, `vec2`, `vec3`, `vec4`, `mat3`, `mat4`. |
| RHI-09 | Uniform locations SHALL be cached per shader (looked up once, reused). |
| RHI-10 | A `ShaderLibrary` SHALL cache compiled shaders by name (`get(name)`, `load(name, path)`). |
| RHI-11 | In Debug builds, `ShaderLibrary` SHALL support reloading a shader from disk on request (hot-reload hook); in Release this MAY be a no-op. |

### 3.3 Textures

| ID | Requirement |
|----|-------------|
| RHI-12 | `OpenGLTexture2D` SHALL create a GPU texture from a `TextureSpec` (width, height, format) and optional pixel data. |
| RHI-13 | Supported formats SHALL include at least `RGBA8` and `RGB8`; the internal/data format mapping SHALL be correct for each. |
| RHI-14 | Filtering modes (`Nearest`, `Linear`) and wrap modes (`Repeat`, `ClampToEdge`) SHALL be settable via `TextureSpec`. |
| RHI-15 | Mipmap generation SHALL be optional via `TextureSpec.generateMips`. |
| RHI-16 | `ITexture2D::bind(slot)` SHALL bind the texture to the given texture unit. |
| RHI-17 | A 1×1 **white texture** SHALL be creatable, used as the default for untextured (flat-color) quads so a single shader path handles both. |

### 3.4 Framebuffers

| ID | Requirement |
|----|-------------|
| RHI-18 | `OpenGLFramebuffer` SHALL be created from a spec (width, height, with a color attachment and an optional depth/stencil attachment). |
| RHI-19 | `IFramebuffer::bind()`/`unbind()` SHALL redirect rendering to/from the framebuffer; `resize(w, h)` SHALL recreate attachments. |
| RHI-20 | `getColorAttachment()` SHALL return a texture id usable as a sampler input (render-to-texture). |
| RHI-21 | Binding an incomplete framebuffer SHALL be detected (`glCheckFramebufferStatus`) and reported via `EMBER_LOG_ERROR`. |

### 3.5 RHI Factory additions

| ID | Requirement |
|----|-------------|
| RHI-22 | `RHI::createTexture2D(spec, pixels)` SHALL return a working `std::shared_ptr<ITexture2D>` (replacing the Epic 02 stub that asserted). |
| RHI-23 | `RHI::createFramebuffer(spec)` SHALL be added, returning `std::shared_ptr<IFramebuffer>`. |
| RHI-24 | Blending SHALL be configurable; `RHI` SHALL enable alpha blending (`GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`) for the 2D pass. |

---

## 4. Texture Loading Requirements

| ID | Requirement |
|----|-------------|
| TEX-01 | A `Texture2DLoader` SHALL load PNG, JPG, and BMP via `stb_image`, returning `std::shared_ptr<ITexture2D>`. |
| TEX-02 | Image loading SHALL respect channel count (RGB vs RGBA) and SHALL flip vertically on load so image-space matches the engine's UV convention. |
| TEX-03 | A load failure (missing/corrupt file) SHALL log an error and return `nullptr`; callers SHALL handle null gracefully (fall back to the white/magenta default). |
| TEX-04 | A built-in **magenta "missing texture"** placeholder SHALL be available and used when a load fails, so missing assets are visually obvious rather than crashing. |
| TEX-05 | `TextureAtlas` SHALL wrap one `ITexture2D` and expose named sub-regions as UV rectangles (`addRegion(name, x, y, w, h)`, `region(name) → UVRect`). |
| TEX-06 | `stb_image` SHALL be compiled into exactly one translation unit (`STB_IMAGE_IMPLEMENTATION`) to avoid duplicate symbols. |

---

## 5. Renderer2D Requirements (`engine/renderer/2d/`)

### 5.1 Frame API

| ID | Requirement |
|----|-------------|
| R2D-01 | `Renderer2D::init()` SHALL set up the shared quad index buffer, the batch vertex buffer, the white texture, and the sprite shader. `Renderer2D::shutdown()` SHALL release them. |
| R2D-02 | `Renderer2D::beginScene(const Camera2D&, const glm::mat4& cameraTransform)` SHALL upload the view-projection matrix and reset per-frame batch state and stats. |
| R2D-03 | `Renderer2D::endScene()` SHALL flush all pending batches (issue the draw calls). |
| R2D-04 | All `draw*` calls SHALL be valid only between `beginScene` and `endScene`; calling them otherwise SHALL trigger `EMBER_ASSERT`. |

### 5.2 Draw calls

| ID | Requirement |
|----|-------------|
| R2D-05 | `drawQuad(position, size, color)` SHALL draw an untextured colored quad (using the white texture internally). |
| R2D-06 | `drawQuad(position, size, texture, tintColor = white, tilingFactor = 1)` SHALL draw a textured quad. |
| R2D-07 | `drawSprite(const glm::mat4& worldTransform, const SpriteRenderer&)` SHALL draw a sprite using the entity's world matrix (from Epic 03's `WorldTransform`), the sprite's texture, color, and flip flags. |
| R2D-08 | A transform-matrix overload of `drawQuad(const glm::mat4&, ...)` SHALL exist so rotation/scale/hierarchy compose correctly. |

### 5.3 Batching

| ID | Requirement |
|----|-------------|
| R2D-09 | Quads SHALL be accumulated into a CPU-side vertex array and uploaded once per flush; the per-vertex layout SHALL be `[vec3 position, vec4 color, vec2 uv, float texIndex, float entityID]`. |
| R2D-10 | A batch SHALL hold up to **4096 quads** and **32 texture slots**; exceeding either SHALL automatically flush and start a new batch. |
| R2D-11 | Within a batch, each distinct texture SHALL occupy one slot; repeated use of the same texture SHALL reuse its slot (no duplicate binds). |
| R2D-12 | The `entityID` vertex attribute SHALL be populated (defaulting to the entity id when drawing a sprite, or a sentinel otherwise) to enable editor picking in Epic 07 at no extra cost now. |
| R2D-13 | `RendererStats { u32 drawCalls, quadCount, vertexCount, textureBinds }` SHALL be tracked per frame and queryable after `endScene`. |

### 5.4 Sorting & transparency

| ID | Requirement |
|----|-------------|
| R2D-14 | Sprites SHALL be drawn ordered by `SpriteRenderer.layer` (ascending integer); within a layer, submission order SHALL be preserved. |
| R2D-15 | Fully/partially transparent sprites (`color.a < 1`) MAY be drawn in a separate back-to-front pass after opaque sprites; at minimum, blending SHALL produce correct results for overlapping translucent quads. |

### 5.5 Components

| ID | Requirement |
|----|-------------|
| R2D-16 | `SpriteRenderer` SHALL contain: a texture reference (`std::shared_ptr<ITexture2D>`, may be null = flat color), `glm::vec4 color`, `i32 layer`, `bool flipX`, `bool flipY`. It SHALL be reflectable/serializable consistent with Epic 03 (texture stored as a path string for serialization). |
| R2D-17 | `Camera2D` SHALL contain: `f32 size` (orthographic half-height in world units), `f32 nearClip`, `f32 farClip`, `bool isPrimary`. |
| R2D-18 | A `SpriteRenderSystem` (Render phase) SHALL iterate entities with `WorldTransform` + `SpriteRenderer` and submit them to `Renderer2D` between begin/endScene. |

### 5.6 Shaders

| ID | Requirement |
|----|-------------|
| R2D-19 | `assets/shaders/sprite.glsl` SHALL render a textured, tinted quad sampling from a `sampler2D[32]` array indexed by the per-vertex `texIndex`. |
| R2D-20 | `assets/shaders/flat_color.glsl` SHALL render an untextured colored quad (or be subsumed by the sprite shader + white texture per R2D-05). |
| R2D-21 | `assets/shaders/text.glsl` SHALL render glyph quads from a font atlas (bitmap alpha or SDF). |
| R2D-22 | All shaders SHALL target GLSL `#version 410 core` (compatible with the macOS 4.1 context and desktop 4.3+). |

---

## 6. Camera Requirements (`engine/renderer/camera/`)

| ID | Requirement |
|----|-------------|
| CAM-01 | A `CameraSystem` (Update phase) SHALL query entities with `Camera2D` + `WorldTransform`, compute the view matrix (inverse of the camera's world transform) and an orthographic projection from `Camera2D.size` and the viewport aspect ratio. |
| CAM-02 | Exactly one camera with `isPrimary == true` SHALL drive rendering; if none or several exist, the system SHALL pick deterministically and log a warning. |
| CAM-03 | The projection SHALL map world units to screen correctly accounting for the framebuffer aspect ratio (no stretching when the window is non-square). |
| CAM-04 | `EditorCamera` SHALL be a non-ECS, free orthographic camera supporting pan and zoom, exposing a view-projection matrix for the editor viewport (Epic 07 consumer). |
| CAM-05 | A world↔screen helper (`screenToWorld` / `worldToScreen`) SHALL be provided for picking and UI. |

---

## 7. Text Rendering Requirements

| ID | Requirement |
|----|-------------|
| TXT-01 | A `FontAsset` SHALL load a TTF via `stb_truetype` and bake a bitmap glyph atlas at a specified pixel size. |
| TXT-02 | The atlas SHALL store per-glyph metrics (advance, bearing, UV rect) for the printable ASCII range at minimum. |
| TXT-03 | `Renderer2D::drawText(text, font, position, pixelSize, color)` SHALL render a left-to-right string as a batch of glyph quads. |
| TXT-04 | Text SHALL be **UTF-8 aware** for codepoint decoding; complex shaping and RTL are explicitly deferred. |
| TXT-05 | `stb_truetype` SHALL be compiled into exactly one translation unit. |

---

## 8. Debug Draw Requirements

| ID | Requirement |
|----|-------------|
| DBG-01 | `DebugDraw::line(a, b, color)`, `rect(position, size, color)`, and `circle(center, radius, color, segments = 32)` SHALL queue immediate-mode line geometry. |
| DBG-02 | `DebugDraw::flush()` SHALL render all queued lines in one (or few) draw call(s) at end of frame using a dedicated line shader (`GL_LINES`). |
| DBG-03 | In Release builds, the `DebugDraw` calls SHALL compile to no-ops (zero runtime cost). |
| DBG-04 | Debug draw SHALL use the active camera's view-projection so debug geometry aligns with world space. |

---

## 9. Tilemap Requirements (Stretch Goal)

> Per the epic's note, the tilemap MAY slip to a later epic if Epic 04 runs long. If implemented:

| ID | Requirement |
|----|-------------|
| TILE-01 | `TilemapComponent` SHALL reference a tileset texture/atlas and a grid of tile ids. |
| TILE-02 | A `TilemapRenderer` SHALL build a vertex buffer for the visible tiles, rebuilding when the tilemap is marked dirty. |
| TILE-03 | Tiles outside the camera's view SHALL be frustum-culled (not submitted). |

---

## 10. Sandbox Requirements (`sandbox/`)

| ID | Requirement |
|----|-------------|
| SBX-01 | A `SpriteStressTest` SHALL create a scene of ≥1000 sprites using ≥8 distinct textures and render them via the ECS `SpriteRenderSystem` + `Renderer2D`. |
| SBX-02 | The sandbox window title (or a debug-draw text overlay) SHALL display `RendererStats` (FPS, draw calls, quad count). |
| SBX-03 | The sandbox SHALL demonstrate a controllable `Camera2D` (pan/zoom) and at least one debug-draw shape and one line of text. |
| SBX-04 | Resizing the window SHALL keep the scene correctly proportioned (aspect-correct projection). |

---

## 11. Test Requirements (`tests/`)

| ID | Requirement |
|----|-------------|
| TST-01 | New tests SHALL join the existing `ember_tests` Catch2 target. |
| TST-02 | Tests that require a GL context SHALL be guarded/skipped on headless CI (no display); pure-CPU logic SHALL always run. |

### 11.1 Required test cases

| Test | Verifies | Req |
|------|----------|-----|
| `Batch_QuadCountAndFlush` | Submitting N quads produces the expected `quadCount` and the batch flushes at the 4096 cap | R2D-10/13 |
| `Batch_TextureSlotReuse` | Drawing with the same texture twice uses one slot; 33 distinct textures force a second batch | R2D-11 |
| `Batch_LayerSorting` | Sprites submitted out of layer order are drawn in ascending-layer order | R2D-14 |
| `Camera_OrthoProjectionAspect` | Projection matrix is aspect-correct; `worldToScreen`∘`screenToWorld` round-trips | CAM-01/03/05 |
| `VertexLayout_StrideAndOffsets` | The batch vertex layout reports the correct stride/offsets for `[pos,color,uv,texIndex,entityID]` | R2D-09 |
| `TextureAtlas_RegionUVs` | `addRegion`/`region` produce correct normalized UV rectangles | TEX-05 |
| `ShaderParse_TypeDirectives` | Splitting a `#type vertex`/`#type fragment` source yields the two stage strings | RHI-07 |

> These are written to avoid needing a live GL context (they test batching/layout/camera/atlas logic on the CPU). On-GPU behavior (actual pixels, framebuffer completeness) is verified by the sandbox `SpriteStressTest` and visual inspection.

---

## 12. Non-Functional Requirements

| ID | Requirement |
|----|-------------|
| NFR-01 | **Throughput.** 1000 sprites using 8 textures SHALL render at ≥60fps on integrated graphics (Intel Iris Xe / Arc class), in **≤8 draw calls** (verified via `RendererStats`). |
| NFR-02 | **No per-quad heap allocation.** The batch vertex buffer SHALL be reused frame-to-frame; submitting quads SHALL not allocate. |
| NFR-03 | **Correct blending.** Overlapping translucent sprites SHALL composite correctly (no order artifacts within the documented sorting rules). |
| NFR-04 | **No UB / leaks.** Sandbox under the `sanitize` preset SHALL produce zero ASan/UBSan reports over a 10-second run and a clean leak report (GPU resources released in destructors). |
| NFR-05 | **Header hygiene.** No OpenGL/glad header SHALL appear in any public `Renderer2D`/camera header (`NFR` consistent with Epic 02 RHI-03). |
| NFR-06 | **Warning-free.** All new code SHALL compile clean under `-Wall -Wextra -Wpedantic -Werror` (GCC/Clang) and `/W4 /WX` (MSVC) on all three CI platforms. |
| NFR-07 | **Resolution independence.** Rendering SHALL be correct across window resizes and HiDPI framebuffer scaling (reuse Epic 02's framebuffer-size viewport handling). |

---

## 13. Out of Scope

Deferred to later epics; SHALL NOT be implemented here:

- Handle-based `AssetManager`/`AssetHandle` and async/hot-reload asset loading — Epic 05 (this epic uses `shared_ptr<ITexture2D>` directly).
- Input handling for camera control beyond a minimal sandbox demo — Epic 05.
- Editor UI, viewport panel, gizmos, entity picking UI — Epic 07 (the `entityID` vertex attribute is laid down now but unused).
- Particle systems, post-processing, render graph, multi-pass effects — future.
- Any 3D rendering, meshes, lighting, materials — Epics 09/10.
- SDF text (bitmap text is sufficient here) unless trivially available.

---

## 14. Acceptance Criteria Summary

The epic is complete when ALL of the following are true:

1. 1000 sprites with 8 textures render at ≥60fps on integrated graphics in ≤8 draw calls (confirmed by `RendererStats`).
2. A `Camera2D` orthographic camera transforms world→screen correctly; pan and zoom work; resizing stays aspect-correct.
3. Text renders a Latin character set from a TTF file.
4. Debug-draw lines/rects/circles appear in the sandbox and compile out in Release.
5. Render-to-texture works: a scene rendered into a framebuffer is displayed as a textured quad.
6. (Stretch) A 32×32 tilemap renders correctly with a 16px tileset.
7. All Catch2 tests pass under debug and release; CI is green on all three platforms with zero warnings.
8. The sandbox runs clean under the `sanitize` preset (no ASan/UBSan reports, no leaks).
