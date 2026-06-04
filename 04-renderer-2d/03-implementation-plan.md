# Epic 04 — 2D Renderer: Implementation Plan

**Status:** Not started  
**Document:** 03-implementation-plan.md  
**Refs:** epic.md, 02-requirements.md, 01-vision.md §11/§9

Work through the phases in order; each ends in a buildable, testable state. Don't proceed until the current phase's boxes are ticked and its verification passes. Requirement IDs (e.g. `R2D-09`) refer to `02-requirements.md`.

**Builds directly on the existing Epic 02 RHI** (`engine/renderer/api/`):

| Already present | Status this epic |
|-----------------|------------------|
| `RHI.h` interfaces `IVertexBuffer/IIndexBuffer/IShader/ITexture2D/IFramebuffer` | extend (layout, framebuffer factory, texture spec) |
| `OpenGLBuffer.cpp` (VB/IB), `OpenGLShader.cpp`, `OpenGLVertexArray.cpp`, `OpenGLContext.cpp` | extend / generalize |
| `createTexture2D` (stubbed — asserts) | implement |
| `drawIndexed(vbo, ibo, count)` (transient VAO, **hard-coded position-only layout**) | generalize to a layout-aware VAO path |

**Tech stack (already fetched):** glm `1.0.1` (`glm::glm`), EnTT `v3.13.2` (`ember_ecs`), `ember_scene` (Transform/WorldTransform), stb (`STB_INCLUDE_DIR` cache var → `stb_image.h`, `stb_truetype.h`), Catch2 `Catch2::Catch2WithMain`.

**Cross-cutting decisions (confirm before starting):**

- **New target `ember_renderer_2d`** holds `2d/`, `camera/`, debug-draw, text, and (for this epic) the texture/font loaders. The asset module (`engine/assets/`) is **Epic 05**; loaders live here for now and relocate later. `SpriteRenderer` holds `std::shared_ptr<ITexture2D>` (not `AssetHandle`) per requirements §1.1.
- **One sprite shader path:** flat-color quads use a 1×1 white texture, so `sprite.glsl` handles both textured and untextured (subsumes `flat_color.glsl`). (Flag if you want them separate.)
- **GL stays out of public headers:** `Renderer2D`/camera/debug headers expose glm + RHI handles only; all GL is behind the RHI and `.cpp` files (NFR-05, consistent with Epic 02 RHI-03).
- **Headless CI:** the required unit tests verify batching/layout/camera/atlas/shader-parse logic on the CPU (no GL context). On-GPU correctness is the sandbox's job.

---

## Phase 0 — Scaffolding & CMake Wiring

Goal: a new `ember_renderer_2d` target configures and links; `assets/shaders/` exists; `cmake --preset debug` succeeds.

### 0.1 Directories

- [x] Create:
  ```
  engine/renderer/2d/include/ember/renderer/
  engine/renderer/2d/src/
  engine/renderer/camera/include/ember/renderer/
  engine/renderer/camera/src/
  engine/renderer/2d/loaders/      # Texture2DLoader, FontAsset (relocate to engine/assets in Epic 05)
  assets/shaders/
  tests/renderer/
  ```

### 0.2 `engine/renderer/2d/CMakeLists.txt`

- [x] Create the target (camera lives under the same lib to avoid target sprawl):
  ```cmake
  add_library(ember_renderer_2d STATIC
      src/Renderer2D.cpp
      src/SpriteBatcher.cpp
      src/DebugDraw.cpp
      src/Text.cpp
      ../camera/src/CameraSystem.cpp
      ../camera/src/EditorCamera.cpp
      loaders/Texture2DLoader.cpp
      loaders/FontAsset.cpp
      systems/SpriteRenderSystem.cpp
  )
  target_include_directories(ember_renderer_2d
      PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
              ${CMAKE_CURRENT_SOURCE_DIR}/../camera/include
      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
              ${STB_INCLUDE_DIR}            # stb_image.h / stb_truetype.h
  )
  target_link_libraries(ember_renderer_2d
      PUBLIC  ember_core ember_ecs ember_scene ember_renderer_api glm::glm
  )
  target_compile_features(ember_renderer_2d PUBLIC cxx_std_20)
  ember_set_compiler_options(ember_renderer_2d)
  ```
  > `STB_INCLUDE_DIR` is `PRIVATE` so stb headers never leak to consumers. Create a `systems/` dir for `SpriteRenderSystem`.

- [x] In `engine/renderer/CMakeLists.txt` add: `add_subdirectory(2d)` (keep `add_subdirectory(api)` first).

### 0.3 Root wiring

- [x] Add `ember_renderer_2d` to the `ember` aggregate `target_link_libraries` in the root `CMakeLists.txt`.

### 0.4 Copy shaders next to the binary

- [x] Decide shader delivery: simplest is to copy `assets/` next to the sandbox exe at build time. Add to `sandbox/CMakeLists.txt`:
  ```cmake
  add_custom_command(TARGET ember_sandbox POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
              ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:ember_sandbox>/assets)
  ```
  (Tests that need shaders should resolve paths the same way, or embed shader source as string literals — see Phase 3.6.)

**Verify Phase 0:** `cmake --preset debug` configures; `ember_renderer_2d` target exists.

---

## Phase 1 — RHI Completion (`engine/renderer/api/`)

Implements `RHI-01 … RHI-24`. This generalizes the Epic 02 stub.

### 1.1 Buffer layout + Vertex Array (RHI-03/04)

- [x] In `RHI.h` add GL-free layout types:
  ```cpp
  enum class ShaderDataType { Float, Float2, Float3, Float4, Int, Mat3, Mat4 };
  u32 shaderDataTypeSize(ShaderDataType);          // bytes
  u32 shaderDataTypeComponentCount(ShaderDataType);

  struct BufferElement {
      ShaderDataType type;
      std::string    name;
      bool           normalized = false;
      u32            size   = 0;   // filled by layout
      u32            offset = 0;   // filled by layout
  };

  class BufferLayout {
  public:
      BufferLayout() = default;
      BufferLayout(std::initializer_list<BufferElement> elems);
      u32 stride() const { return m_stride; }
      const std::vector<BufferElement>& elements() const { return m_elements; }
  private:
      std::vector<BufferElement> m_elements;
      u32 m_stride = 0;            // computed in ctor
  };
  ```
- [x] Extend `IVertexBuffer` with `setLayout(const BufferLayout&)` / `getLayout()`.
- [x] Add an `IVertexArray` interface: `bind()/unbind()`, `addVertexBuffer(shared_ptr<IVertexBuffer>)` (applies its layout via `glVertexAttribPointer` using element offsets/stride), `setIndexBuffer(shared_ptr<IIndexBuffer>)`, `indexCount()`.
- [x] Add `RHI::createVertexArray() → std::shared_ptr<IVertexArray>` and a draw overload `RHI::drawIndexed(const std::shared_ptr<IVertexArray>&, u32 indexCount = 0)` (0 = use the VAO's index count).
- [x] Implement `OpenGLVertexArray` against this interface (replace the Epic 02 transient/hard-coded version): one GL VAO, iterate the VBO layout to set attribute pointers, map `ShaderDataType → GL_FLOAT/GL_INT` + component count.
- [x] Keep the old `drawIndexed(vbo, ibo, count)` working (the sandbox triangle uses it) or migrate the triangle to the new VAO path.

### 1.2 Dynamic vertex buffers (RHI-01/02)

- [x] Add a `OpenGLVertexBuffer(u32 sizeBytes)` ctor that allocates an empty `GL_DYNAMIC_DRAW` store; `setData(data, size)` uses `glBufferSubData` (or `glNamedBufferSubData`) without reallocating.
- [x] Add `RHI::createVertexBuffer(u32 sizeBytes)` (dynamic, empty) overload.

### 1.3 Shader uniforms + ShaderLibrary (RHI-06 … RHI-11)

- [x] Confirm `OpenGLShader` uniform setters cover `int`, `int[]` (`glUniform1iv` — for the sampler array), `float`, `vec2/3/4`, `mat3`, `mat4`. Add any missing (likely `setIntArray`, `setMat3`).
- [x] Add a `#type` parser: split a single `.glsl` source on `#type vertex` / `#type fragment` into two stage strings; add `RHI::createShaderFromFile(path)` and/or `RHI::createShader(name, combinedSrc)`.
- [x] Implement `ShaderLibrary` (`engine/renderer/api/` or `2d/`): `load(name, path)`, `get(name)`, `exists(name)`, plus `reload(name)` active only in Debug (`#if !defined(NDEBUG)`).

### 1.4 Textures (RHI-12 … RHI-17, RHI-22)

- [x] Extend `TextureSpec` in `RHI.h`:
  ```cpp
  enum class TextureFormat { RGBA8, RGB8, R8 };
  enum class TextureFilter { Nearest, Linear };
  enum class TextureWrap   { Repeat, ClampToEdge };
  struct TextureSpec {
      u32 width = 1, height = 1;
      TextureFormat format = TextureFormat::RGBA8;
      TextureFilter filter = TextureFilter::Linear;
      TextureWrap   wrap   = TextureWrap::Repeat;
      bool generateMips = false;
  };
  ```
- [x] Create `OpenGLTexture2D` (`opengl/include` + `opengl/src`): `glCreateTextures`/`glTexImage2D`, map format/filter/wrap enums to GL, upload pixels, `bind(slot)` via `glBindTextureUnit`, `glGenerateMipmap` when requested. Destructor `glDeleteTextures`.
- [x] Implement `RHI::createTexture2D(spec, pixels)` (replace the asserting stub).
- [x] Add `RHI::whiteTexture()` returning a cached 1×1 RGBA8 white texture (RHI-17).

### 1.5 Framebuffers (RHI-18 … RHI-21, RHI-23)

- [x] Add `FramebufferSpec { u32 width, height; bool depth = true; }` to `RHI.h`.
- [x] Create `OpenGLFramebuffer`: color texture attachment (+ optional depth renderbuffer/texture), `bind()` sets viewport to fbo size, `unbind()` restores default fbo, `resize(w,h)` recreates, `getColorAttachment()` returns the color texture id, `glCheckFramebufferStatus` logged on failure.
- [x] Add `RHI::createFramebuffer(spec) → std::shared_ptr<IFramebuffer>`.

### 1.6 Pipeline state (RHI-24)

- [x] Add `RHI::setBlend(bool)` enabling `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`; enable it during the 2D pass.

### 1.7 CMake

- [x] Add the new `.cpp` files (`OpenGLTexture2D.cpp`, `OpenGLFramebuffer.cpp`) to `engine/renderer/api/CMakeLists.txt`.

**Verify Phase 1:** `cmake --build --preset debug --target ember_renderer_api` compiles clean; the sandbox triangle still renders.

---

## Phase 2 — Texture Loading (`engine/renderer/2d/loaders/`)

Implements `TEX-01 … TEX-06`.

- [x] Create `loaders/stb_impl.cpp` defining `STB_IMAGE_IMPLEMENTATION` + `#include <stb_image.h>` (exactly one TU — TEX-06). Add `STB_TRUETYPE_IMPLEMENTATION` here too (Phase 5) or in a separate TU.
- [x] `Texture2DLoader::load(path) → std::shared_ptr<ITexture2D>` (TEX-01/02): `stbi_set_flip_vertically_on_load(true)`, `stbi_load`, choose `TextureFormat` by channel count, build a `TextureSpec`, call `RHI::createTexture2D`, `stbi_image_free`.
- [x] On failure: `EMBER_LOG_ERROR`, return the magenta placeholder (TEX-03/04). Provide `Texture2DLoader::missingTexture()` (cached 2×2 magenta/black checker).
- [x] `TextureAtlas` (`include/ember/renderer/TextureAtlas.h`): wraps `shared_ptr<ITexture2D>`; `addRegion(name, x, y, w, h)` stores pixel rect; `region(name) → UVRect{u0,v0,u1,v1}` normalized by texture size (TEX-05).

**Verify Phase 2:** unit test `TextureAtlas_RegionUVs` (pure CPU) passes; loading a PNG in the sandbox shows the image (Phase 8).

---

## Phase 3 — Renderer2D Sprite Batcher (`engine/renderer/2d/`)

Implements `R2D-01 … R2D-22`.

### 3.1 Vertex + batch state

- [x] Define the batch vertex (R2D-09):
  ```cpp
  struct QuadVertex {
      glm::vec3 position;
      glm::vec4 color;
      glm::vec2 uv;
      f32       texIndex;
      f32       entityID;
  };
  ```
  with a matching `BufferLayout{ {Float3,"a_Position"},{Float4,"a_Color"},{Float2,"a_UV"},{Float,"a_TexIndex"},{Float,"a_EntityID"} }`.
- [x] `SpriteBatcher` owns: a dynamic VBO sized `4096*4*sizeof(QuadVertex)`, a VAO, a shared 4096-quad index buffer (pattern `0,1,2,2,3,0`), a CPU vertex vector, a `std::array<shared_ptr<ITexture2D>,32>` texture-slot table (slot 0 = white), and a running `RendererStats`.

### 3.2 Frame API (R2D-01..04)

- [x] `Renderer2D::init()/shutdown()` (R2D-01): build the index buffer, VBO/VAO, white texture, load `sprite.glsl` (embedded string fallback for headless tests — Phase 3.6).
- [x] `beginScene(const Camera2D&, const glm::mat4& cameraWorldTransform)` (R2D-02): compute view = `inverse(cameraWorldTransform)`, projection from `Camera2D` + aspect, set `u_ViewProjection`, reset batch + stats, bind blend.
- [x] `endScene()` (R2D-03): flush.
- [x] All `draw*` between begin/end; otherwise `EMBER_ASSERT` (R2D-04) — guard with an internal `m_inScene` flag.

### 3.3 Draw + batching (R2D-05..13)

- [x] `drawQuad(pos, size, color)` and textured overload (R2D-05/06); `drawQuad(const glm::mat4&, color)` and textured (R2D-08); `drawSprite(worldMatrix, SpriteRenderer)` (R2D-07) — applies flipX/flipY by swapping UVs.
- [x] Internal `submitQuad(transform, color, texture, tilingFactor, entityID)`: resolve the texture's slot (reuse if already bound this batch, else assign next slot); if quad count == 4096 **or** no free texture slot → `flush()` then continue (R2D-10/11). Append 4 vertices with the unit-quad positions transformed by `transform`, UVs (× tiling), color, texIndex, entityID (R2D-12).
- [x] `flush()`: upload the CPU vertex span via `setData`, bind each used texture to its unit, set the `u_Textures[32]` sampler array uniform to `{0..31}`, `RHI::drawIndexed(vao, quadCount*6)`; bump `RendererStats.drawCalls` (R2D-13); reset batch.
- [x] `stats() → const RendererStats&` (R2D-13).

### 3.4 Sorting & transparency (R2D-14/15)

- [x] `SpriteRenderSystem` collects `(WorldTransform, SpriteRenderer)` tuples, **stable-sorts by `layer`**, then submits in order (R2D-14). Optionally split opaque vs `color.a < 1` into two passes (opaque first) (R2D-15).

### 3.5 Components + system (R2D-16/17/18)

- [x] `include/ember/renderer/Components2D.h`:
  ```cpp
  struct SpriteRenderer {
      std::shared_ptr<ITexture2D> texture;   // null = flat color
      std::string texturePath;               // for serialization (Epic 03 reflection)
      glm::vec4 color{1.0f};
      i32  layer  = 0;
      bool flipX = false, flipY = false;
  };
  struct Camera2D { f32 size = 5.0f; f32 nearClip = -1.0f, farClip = 1.0f; bool isPrimary = true; };
  ```
  Add `EMBER_REFLECT` for the serializable fields (texturePath, color, layer, flipX, flipY; size/clip/isPrimary) — `shared_ptr<ITexture2D>` is **not** reflected (rebuilt from `texturePath` on load).
- [x] `systems/SpriteRenderSystem.{h,cpp}`: an `ISystem` (Render phase) that calls `Renderer2D::drawSprite` for each sprite (assumes `beginScene` already called for the frame, or wraps it — document the contract).

### 3.6 Shaders (R2D-19..22)

- [x] `assets/shaders/sprite.glsl` (`#version 410 core`, `#type vertex`/`#type fragment`): VS passes color/uv/texIndex; FS samples `uniform sampler2D u_Textures[32]` indexed by a flat `int(texIndex)`, multiplies by color. (Use a `switch`/if-chain to index the sampler array — required on GL ≤4.5 where dynamic non-uniform indexing is unsupported.)
- [x] Keep an **embedded copy** of the sprite shader source as a string constant so `Renderer2D` and tests work without the on-disk file; `ShaderLibrary` can override from disk in debug.

**Verify Phase 3:** `Batch_QuadCountAndFlush`, `Batch_TextureSlotReuse`, `Batch_LayerSorting`, `VertexLayout_StrideAndOffsets`, `ShaderParse_TypeDirectives` pass (CPU-only).

---

## Phase 4 — Camera (`engine/renderer/camera/`)

Implements `CAM-01 … CAM-05`.

- [x] `Camera2D` lives in `Components2D.h` (Phase 3). Add a `CameraSystem` (`ISystem`, Update phase): find the primary `Camera2D` + `WorldTransform`; view = `inverse(worldTransform)`; projection = `glm::ortho(-size*aspect, size*aspect, -size, size, near, far)` (CAM-01/03). Cache the view-projection for `Renderer2D::beginScene`.
- [x] Pick exactly one `isPrimary` camera; warn (`EMBER_LOG_WARN`) on none/multiple, choose deterministically (CAM-02).
- [x] `EditorCamera` (non-ECS): `pan(delta)`, `zoom(amount)`, `setViewportSize(w,h)`, `viewProjection()` (CAM-04).
- [x] `screenToWorld(px, viewportSize, viewProj)` / `worldToScreen(...)` free helpers (CAM-05).

**Verify Phase 4:** `Camera_OrthoProjectionAspect` passes (matrix math + round-trip, CPU-only).

---

## Phase 5 — Text Rendering

Implements `TXT-01 … TXT-05`.

- [x] Add `STB_TRUETYPE_IMPLEMENTATION` to a single TU (Phase 2's `stb_impl.cpp` or its own).
- [x] `FontAsset::loadFromFile(path, pixelSize)` (TXT-01/02): `stbtt_BakeFontBitmap` (or packed atlas) into a single-channel bitmap; create an `R8` `ITexture2D`; store per-glyph metrics (advance, bearing, UV rect) for printable ASCII (extendable to more codepoints).
- [x] `Renderer2D::drawText(text, font, position, pixelSize, color)` (TXT-03/04): UTF-8 decode codepoints, advance the pen, submit one quad per glyph through the sprite batcher using the font atlas texture and the glyph UVs.
- [x] `assets/shaders/text.glsl`: samples the glyph alpha (R8) and multiplies by color (bitmap path; SDF deferred).

**Verify Phase 5:** sandbox renders a line of text (visual); a CPU test can check UTF-8 decode + glyph advance accumulation if factored out.

---

## Phase 6 — Debug Draw

Implements `DBG-01 … DBG-04`.

- [x] `DebugDraw` (`include/ember/renderer/DebugDraw.h`): `line(a,b,color)`, `rect(pos,size,color)` (4 lines), `circle(center,radius,color,segments=32)` (segment lines); accumulate into a CPU line-vertex buffer.
- [x] `flush()`: upload + `RHI::drawIndexed`/`glDrawArrays(GL_LINES)` via a dedicated `line.glsl`; uses the active camera's view-projection (DBG-02/04).
- [x] Compile out in Release: wrap the public calls so they are no-ops when `NDEBUG` is defined (DBG-03) — e.g. inline `#if defined(NDEBUG)` empty bodies, mirroring `EMBER_ASSERT`.

**Verify Phase 6:** debug shapes visible in the sandbox debug build; absent in release.

---

## Phase 7 — Tilemap (Stretch — `TILE-01..03`)

> Skippable / movable to Epic 08 per the epic note.

- [ ] `TilemapComponent { shared_ptr<ITexture2D> tileset; u32 tileSize; u32 cols, rows; std::vector<u32> tiles; bool dirty; }`.
- [ ] `TilemapRenderer` system: build a vertex buffer of visible tile quads, rebuild on `dirty`, frustum-cull tiles outside the camera rect (TILE-02/03).

---

## Phase 8 — Sandbox, Tests & Verification

### 8.1 Sandbox stress test (`SBX-01..04`)

- [ ] `sandbox/SpriteStressTest.cpp` (new exe target or a mode of `main.cpp`): create a `Scene`, spawn ≥1000 entities with `Transform`/`WorldTransform` + `SpriteRenderer` across ≥8 textures; run scheduler (TransformSystem → CameraSystem → SpriteRenderSystem) each frame between `Renderer2D::beginScene/endScene`.
- [ ] Display `RendererStats` (FPS, draw calls, quads) in the title bar and/or via `drawText` (SBX-02).
- [ ] Wire camera pan/zoom (keyboard/mouse via GLFW for now), one debug shape, one text line (SBX-03); aspect-correct on resize (SBX-04, reuse Epic 02 framebuffer-size viewport).

### 8.2 Tests

- [x] Add `tests/renderer/TestSpriteBatch.cpp` (+ camera/atlas/shader-parse tests) to `ember_tests` sources. Keep them **GL-context-free** (test the batcher's CPU bookkeeping by exposing counts/slots, the layout's stride/offsets, camera matrices, atlas UVs, shader `#type` split).
- [x] Confirm `ctest --preset debug` runs them; release skips any GL-dependent ones (none required to be GL-dependent).

### 8.3 Verification (NFR-01 … NFR-07)

- [ ] `cmake --build --preset debug && ctest --preset debug` green.
- [ ] Sandbox: 1000 sprites / 8 textures at ≥60fps in ≤8 draw calls (read `RendererStats`) — NFR-01.
- [ ] `cmake --preset sanitize && cmake --build --preset sanitize` then run the sandbox ~10s: zero ASan/UBSan, clean leaks (GPU resources freed in destructors) — NFR-04.
- [ ] Header hygiene: no GL/glad include in any public `2d/`/`camera/` header — NFR-05.
  ```sh
  ! grep -rIn "glad/\|GL/gl" engine/renderer/2d/include engine/renderer/camera/include
  ```
- [ ] Push; CI green on Windows/MSVC, Ubuntu/GCC, macOS/Clang, zero warnings — NFR-06.

### 8.4 Acceptance checklist (from `02-requirements.md` §14)

- [ ] 1000 sprites / 8 textures ≥60fps in ≤8 draw calls.
- [ ] `Camera2D` world→screen correct; pan/zoom; aspect-correct resize.
- [ ] Latin text from a TTF renders.
- [ ] Debug draw visible in debug, compiled out in release.
- [ ] Render-to-texture: scene → framebuffer → displayed as a quad.
- [ ] (Stretch) 32×32 tilemap with a 16px tileset.
- [ ] All tests pass (debug + release); CI green, zero warnings; sanitize clean.

---

## Suggested Commit Sequence

1. `Epic 04: scaffold renderer_2d target + CMake wiring` (Phase 0)
2. `Epic 04: RHI completion — layout/VAO, dynamic VBO, textures, framebuffers, shader lib` (Phase 1)
3. `Epic 04: texture loading + atlas (stb_image)` (Phase 2)
4. `Epic 04: Renderer2D sprite batcher + components + system` (Phase 3)
5. `Epic 04: camera system + editor camera` (Phase 4)
6. `Epic 04: TTF text rendering` (Phase 5)
7. `Epic 04: debug draw` (Phase 6)
8. `Epic 04: (stretch) tilemap` (Phase 7)
9. `Epic 04: sprite stress test, tests, verification` (Phase 8)

Each commit should leave the build green and CI passing.
