# Epic 07 — Editor v1: Implementation Plan

**Status:** Complete (all phases 0-9) — builds & runs on Windows (MinGW) and macOS (clang)  
**Document:** 03-implementation-plan.md  
**Refs:** epic.md, 02-requirements.md

Phases are ordered so each one ends **buildable and demoable**: scaffold → viewport → hierarchy → inspector → undo/redo → save/load → play/stop → picking → asset browser → tests. The editor is a single new application target (`editor/`); **no `ember_*` engine library gains an ImGui dependency**. The one engine-side change is an opt-in RHI framebuffer ID attachment for picking (Phase 7), kept GL-internal. Requirement IDs (e.g. `INS-02`) refer to `02-requirements.md`.

> **Build/verify reality (unchanged from prior epics).** The editor is GLFW + OpenGL + ImGui, so it **only builds/runs in the user's MinGW/GCC-13 desktop build** — the Linux sandbox has no GL/GLFW (`glfw_stub`) and EnTT codegen exceeds its timeout. In-sandbox we verify the **engine-independent logic** (CommandHistory, command undo/redo, play/stop restore via serialization, the reflection inspect model, picking math) with headless Linux-native drivers + Catch2, and `-Werror` syntax-check the GL/ImGui TUs against clean headers. Visual/interaction acceptance is the user's build. The recurring **mount truncation** of freshly-edited headers may require reconstructing clean copies for syntax checks (as in Epics 05/06/12).

**Grounding (current repo state):**

| Fact | Consequence |
|------|-------------|
| ImGui `v1.90.9-docking` is **fetched but only populated** (no `add_subdirectory`); `IMGUI_SOURCE_DIR` is a cache var; `stb` populated too | the `editor/` target compiles ImGui core (`imgui*.cpp`) + `backends/imgui_impl_glfw.cpp` + `imgui_impl_opengl3.cpp` itself, warnings-suppressed |
| `Window::getNativeHandle() → GLFWwindow*`, `setEventCallback`, `getWidth/Height`, `setTitle` (02) | ImGui GLFW backend init; viewport/title wiring |
| `IFramebuffer{ bind/unbind/resize/getColorAttachment() }`, `FramebufferSpec{ width,height,depth }`, `RHI::createFramebuffer` (04) | scene→FBO→`ImGui::Image` works **as-is**; picking needs a new opt-in ID attachment + readback (Phase 7) |
| `Renderer2D` vertex has a per-vertex `entityID` float; drawQuad plumbs it; shader currently writes color only | GPU picking adds a 2nd fragment output + MRT; or use the CPU fallback (no shader change) |
| `ComponentMeta{ name, type, serialize, deserialize, drawUI(World&,Entity) }`; `drawUI` is **reserved/null this epic**; `ComponentRegistry::all()/byName/byType` (03) | inspector + "Add Component" list; editor keeps its **own** type→draw table (registry is const, ImGui must not enter engine libs) |
| `EMBER_REFLECT(T){ EMBER_FIELD(x); }` + YAML read/write visitors cover f32/i32/u32/u64/bool/string/vec2-4/Entity/AssetHandle/enum (03/05/06) | the `ImGuiInspectVisitor` mirrors that exact type coverage |
| `SceneManager(EventBus&)` with injected `setLoader(Loader)`, `load/getActive/transition/endFrame` (03) | Open wires loader → `YAMLDeserializer::deserialize` |
| `YAMLSerializer::serialize(const Scene&)→string`, `YAMLDeserializer::deserialize(string,Scene&)`, `serializePrefab` (05) | Save writes the string; Play/Stop snapshot is the same string in memory |
| `World`: `create/destroy/emplace/get/tryGet/view`, `Tag`, `Transform`, `Parent`/`Children`, `WorldTransform`+`TransformSystem`, `Scene::create/getWorldTransform` (03) | hierarchy, selection, reparent, create/delete |
| `EditorCamera` (`Camera.h`): `setViewportSize`, `zoom`, pan, `viewProjection()` (04); `Input` mouse facade (05) | Edit-mode viewport camera |

**Cross-cutting decisions (confirm before starting):**

- **No engine library depends on ImGui.** ImGui lives only in `editor/`. The inspector's per-type draw table is **editor-side** (an `EMBER_REGISTER_INSPECTOR(T, fn)` table + a reflection default), not `ComponentMeta.drawUI` (which stays null — it's in `ember_ecs` and `all()` is const).
- **Mutations go through `CommandHistory`.** Phases 2/3 build hierarchy/inspector with direct mutations first to stay demoable, then **Phase 4 retrofits** every mutation into commands. Continuous ImGui drags coalesce into one command via `IsItemActivated()`→`IsItemDeactivatedAfterEdit()`.
- **Play/Stop = serialize snapshot.** Entering Play does `YAMLSerializer::serialize(active)` into a `std::string`; Stop does `YAMLDeserializer::deserialize(snapshot, active)`. No bespoke deep-copy; reuses the tested round-trip.
- **Picking: GPU primary, CPU fallback, one interface.** Panels call `pickEntity(vpX,vpY)→Entity`. Primary impl = GL ID attachment (`FramebufferSpec.idAttachment`, MRT, `readPixelInt`); fallback = CPU ray-vs-AABB over `WorldTransform`-projected bounds. The CPU path lets Phases 1-6 proceed before the RHI work lands.
- **File dialog = in-ImGui browser by default.** No new native dependency; NFD vendoring is optional polish (open item).
- **Module/target deps (no cycles):** `ember_editor` (app) → `ember` aggregate + ImGui(core+backends) + GLFW + glad. Engine libs unchanged except RHI (Phase 7).

---

## Phase 0 — Editor app scaffold + ImGui layer

Implements `SHELL-01 … SHELL-05`.

- [x] `editor/` tree: `main.cpp`, `EditorApplication.h/.cpp`, `ImGuiLayer.h/.cpp`, `EditorContext.h` (shared, ImGui-free panel state). `editor/CMakeLists.txt` → `ember_editor`. (Placeholder panels are drawn inline in `EditorApplication::drawPanels` for now; real panel classes arrive in their phases.)
- [x] CMake: `imgui` static lib from `${IMGUI_SOURCE_DIR}` (core 4 TUs + `imgui_demo` + `imgui_impl_glfw`/`imgui_impl_opengl3`), `SYSTEM PUBLIC` include dirs, links `glfw`, **no `-Werror`** (third-party); `ember_editor` links `ember` + `imgui` (glad/glfw transitive). Root `add_subdirectory(editor)` added; `editor/CMakeLists.txt` self-guards (`return()` when `glfw`/`IMGUI_SOURCE_DIR` are absent — e.g. the headless sandbox).
- [x] `ImGuiLayer`: `init` (CreateContext, `DockingEnable`+`ViewportsEnable`, `IniFilename = editor/layout.ini`, JetBrains Mono 14px if present, GLFW+OpenGL3 backends via `getNativeHandle()`/`#version 410`), `begin()`, `end()` (render draw data + multi-viewport update/restore), `shutdown()`.
- [x] `EditorApplication`: owns `Window`, `RHI::init`/`Renderer2D::init`, `AssetManager`, `EventBus`, `SceneManager`, active `Scene`, `ImGuiLayer`, `EditorContext`. Loop: poll → clear backbuffer → imgui begin → dockspace+menu → panels → imgui end → swap. (`CommandHistory` slot added in Phase 4.)
- [x] Fullscreen dockspace host window (`PassthruCentralNode`) + menu-bar stub (File/Window, items disabled until Phase 5) + layout persisted to `editor/layout.ini` via ImGui's `IniFilename`. "Reset Layout" item present (disabled stub).

**Verify Phase 0:** ✅ the engine-side API surface the editor calls (`Window`/`RHI`/`Renderer2D`/`Scene`/`SceneManager`/`AssetManager`/`EventBus`/`Timer`/`Log` + `EditorContext.h`) compiles `-Wall -Wextra -Wpedantic -Werror` against clean headers. The ImGui TUs (`ImGuiLayer.cpp`, `EditorApplication.cpp`) use stable ImGui 1.90-docking API and build in the user's desktop environment (ImGui isn't fetched in the sandbox); `editor/CMakeLists.txt` configures the `imgui` + `ember_editor` targets there.

---

## Phase 1 — Viewport panel: scene → framebuffer → ImGui::Image

Implements `VP-01, VP-02, VP-03, VP-05` (picking is Phase 7).

- [x] `panels/ViewportPanel.h/.cpp`: owns an `IFramebuffer` (`RHI::createFramebuffer`), created lazily and `resize()`d when the content-region size changes (clamped to ≥1×1 so a tabbed-away/zero-size panel is safe); keeps `EditorCamera::setViewportSize` in sync.
- [x] Render path: bind FBO → `setViewport` → clear to the scene's background color → `TransformSystem` (rebuild dirty `WorldTransform`) → `Renderer2D::beginScene(camera.viewProjection())` → tilemap + sprite + particle systems → `endScene` → unbind. Shown via `ImGui::Image(getColorAttachment(), …, {0,1},{1,0})` (V-flip for GL origin), filling the panel.
- [x] Edit-mode camera: scroll = proportional `zoom`; middle-drag / Alt+left-drag = `pan`. Active only while the image is hovered; world-per-pixel derived from the projection's `vp[1][1]` (no `EditorCamera` API change). Particles run with `dt=0` in Edit mode (draw, don't simulate).
- [~] Play-mode input routing (VP-03): deferred to Phase 6 (the PlayModeController owns the runtime `InputManager`); the panel already gates camera input on hover so it's inert during play.
- [x] Overlay (VP-05): `Renderer2D::stats()` (draw calls / quads) drawn as overlay text at the image's top-left via the window draw list.

**Verify Phase 1:** ✅ the panel's engine API usage (FBO create/resize/bind, `setViewport`/`clear`/`setClearColor`, `TransformSystem` + tilemap/sprite/particle systems, `beginScene`/`endScene`, `getColorAttachment`, `EditorCamera`, `RendererStats`) compiles `-Werror` against clean headers — no engine change. The ImGui draw path + visual render are the user build.

---

## Phase 2 — Scene Hierarchy panel (direct mutation first)

Implements `HIER-01 … HIER-06` (commands retrofitted in Phase 4).

- [x] `panels/SceneHierarchyPanel.h/.cpp`: walks `SceneOps::roots` then recurses `childrenOf` as `ImGui::TreeNodeEx` (leaf flag when childless, `SpanAvailWidth`); label = `Tag.name` or `Entity <id>`.
- [x] Selection: click (ignoring the open/close arrow toggle) sets `ctx.selected`; selected row gets the `Selected` flag; clicking empty space clears.
- [x] Context menu on item (Create Empty/Sprite/Camera, Duplicate, Delete) **and** on the panel background (Create at root). Item-created entities are parented under the right-clicked entity; created/duplicated entity becomes the selection.
- [x] Reparent via `ImGui` drag-drop payload `"EMBER_ENTITY"` (entity id); drop on an entity → reparent under it; drop on the background dummy → un-parent. Cycle/self guards in `SceneOps::reparent`.
- [x] Inline rename: double-click → `InputText` (auto-select, focus grabbed) over the row; commits on Enter or focus-loss via `SceneOps::rename`.
- [x] Delete removes the subtree (`Scene::destroy`, which detaches from parent + orphans children); clears selection if it pointed at the deleted entity. All mutations are **deferred** to after the tree walk to keep iteration valid, and set `ctx.dirty`.

**Verify Phase 2:** ✅ `SceneOps.cpp` (the ImGui-free mutation layer) compiles `-Wall -Wextra -Wpedantic -Werror`; its behavior is thin wrappers over `Scene::create/setParent/getParent/destroy` (covered by Epic 03 hierarchy tests) + the prefab duplicate round-trip (Epic 05/06). A dedicated `SceneOps` Catch2 test lands in Phase 9. The ImGui tree interaction is the user build.

---

## Phase 3 — Inspector panel: reflection-driven + overrides

Implements `INS-01 … INS-04, INS-06` (command routing in Phase 4).

- [x] `editor/ImGuiInspectVisitor.h`: reflection visitor (same `operator()(name, T&)` shape as the YAML visitor) emitting widgets — `f32`→`DragFloat`, `i32`→`DragInt`, `u32/u64`→`DragScalar`, `bool`→`Checkbox`, `std::string`→`InputText`, `glm::vec2/3/4`→`DragFloatN`, `Entity`/`vector<Entity>`→read-only text, `AssetHandle<T>`→id text (picker in the SpriteRenderer override), enum→`InputInt`. Tracks `changed()` for dirty-flagging.
- [x] `editor/InspectorRegistry.h/.cpp` + `registerInspector<T>(name, draw)`: an editor-side table (`InspectorEntry{name,type,has,add,remove,draw}`) — `has/add/remove` are ImGui-free `World` thunks; `draw` carries the ImGui field UI. Idempotent by type. (Used instead of the const engine `ComponentMeta.drawUI`, keeping ImGui out of engine libs.)
- [x] `panels/InspectorPanel.h/.cpp`: for the selected entity, draws each present component (`has`) as a collapsing header → its `draw`; header right-click → Remove Component (deferred); bottom "Add Component" → filtered popup of registered types where `!has` → `add`. Any change sets `ctx.dirty` + `markTransformDirty`.
- [x] Built-in overrides (`EditorInspectors.cpp`): **Transform** (vec3 pos, rotation degrees↔radians, vec3 scale, per-field reset buttons), **SpriteRenderer** (texturePath field, `ColorEdit4`, layer, flipX/Y, uvRect), **Camera2D** (size, near/far, isPrimary). Reflection defaults registered for Tag, SpriteAnimator, ParticleEmitter, RigidBody2D, Box/CircleCollider2D.
- [~] Read-only-when-Paused badge (INS-06) lands with Phase 6 (the play/pause state lives in `PlayModeController`); the inspector edits the active scene as usual until then.

**Verify Phase 3:** ✅ the ImGui-free registry layer — container dedupe, `byType`, and the "Add Component = entries where `!has`" filtering (TST-06) — passes a headless driver; the `registerInspector<T>` `has/add/remove` thunks compile clean. The `ImGuiInspectVisitor` mirrors the YAML visitor's exact field-type set (so `reflect<T>` instantiates for every registered component), verified against clean headers; the ImGui widgets + live viewport update are the user build. (The recording-visitor TST-04 lands in Phase 9.)

---

## Phase 4 — Command history (undo/redo) + retrofit mutations

Implements `CMD-01 … CMD-08`, `INS-05`.

- [x] `editor/CommandHistory.h/.cpp` (ImGui-free): `Command` interface (`execute/undo/name`) + `FunctionalCommand` (do/undo lambdas); linear stack capped at 100 (oldest evicted), redo-tail truncation on new push, `push`/`pushExecuted`/`undo`/`redo`/`canUndo`/`canRedo`/`clear`/`size`.
- [x] `editor/Commands.h/.cpp` factories: `create` (Empty/Sprite/Camera, optional parent), `remove` (serializes the sub-tree to a prefab string so undo re-instantiates it; round-trips across redo), `duplicate`, `reparent` (old/new parent), `rename` (old/new), `setComponent` (type-erased before/after via the InspectorEntry snapshot/restore thunks).
- [x] Retrofit: Hierarchy create/duplicate/delete/reparent/rename now push commands; Inspector field edits coalesce (snapshot the pre-edit value, begin on first `changed`, commit one `setComponent` via `pushExecuted` when `!IsAnyItemActive()`), and Add/Remove Component are undoable `FunctionalCommand`s (Remove's undo re-adds + restores the value). `InspectorEntry` gained `snapshot`/`restore` thunks.
- [x] Shortcuts in `EditorApplication::handleShortcuts`: `Ctrl+Z` undo, `Ctrl+Y` / `Ctrl+Shift+Z` redo (suppressed while a text field has focus). Mutations set `ctx.dirty`; create/duplicate update selection.

**Verify Phase 4:** ✅ `CommandHistory` passes a headless driver — execute/undo/redo, redo-tail truncation, cap eviction, and `pushExecuted` (records without executing). `Commands.cpp` + the retrofitted panels compile `-Wall -Wextra -Wpedantic -Werror` against clean headers. Catch2 versions (`TestCommandHistory`, structural-command undo-to-prior-state) land in Phase 9; ImGui drag-coalescing is the user build.

---

## Phase 5 — Scene save/load, menu bar, shortcuts, dirty flag

Implements `FILE-01 … FILE-04`, `KEY-01`.

- [x] Startup wiring: `registerRenderComponents()` + `registerPhysics2DComponents()` (core components self-register via the serializer's `ensureRegistered()`) + `installAssetSerializationResolver(m_assets)` so Save/Load round-trips sprites/physics/handles. (Open/Save go straight through `YAMLSerializer`/`Deserializer`; the `SceneManager` loader hook is available but not needed for v1.)
- [x] File menu in the dockspace menu bar: New (Ctrl+N), Open… (Ctrl+O), Save (Ctrl+S), Save As… (Ctrl+Shift+S), Exit. Save = `YAMLSerializer::saveToFile`; Open = `YAMLDeserializer::loadFromFile` into a fresh `Scene` that then becomes active (panels follow `ctx.scene`).
- [x] In-ImGui file browser (`editor/FileBrowser.h/.cpp`): modal rooted at `assets/`, dir navigation + `.escene` filter, filename field for Save; one helper drives both Open and Save-As (NFD remains optional, §13).
- [x] Dirty tracking: panel mutations set `ctx.dirty`; Save clears it; the window title shows `Ember Editor — <name> *` (updated only when it changes). (The unsaved-changes confirm-modal on New/Open is the one deferred sub-item — currently New/Open proceed directly.)
- [x] Global shortcuts (gated on `!io.WantTextInput`): Undo/Redo, Save, Save As, New, Open, `Ctrl+D` Duplicate (selection), `Delete` (selection) — all via commands. `F` focus-on-selection and `Ctrl+P` toggle-Play land with Phases 7/6 respectively.

**Verify Phase 5:** ✅ the engine-side file API the editor calls (`saveToFile`/`loadFromFile`, `registerRenderComponents`/`registerPhysics2DComponents`, `installAssetSerializationResolver`, `Result` bool/`error()`, `std::filesystem` name) compiles `-Werror` against clean headers; the serialize→deserialize round-trip itself is covered by Epic 05 tests (a TST-03 editor-created-entity case is added in Phase 9). The browser/menu interaction is the user build, which now compiles & links the editor clean.

---

## Phase 6 — Play / Edit / Pause state machine

Implements `PLAY-01 … PLAY-06`, completes `VP-03`.

- [x] `editor/PlayModeController.h/.cpp`: `Mode{Edit,Playing,Paused}`; `play()` snapshots `YAMLSerializer::serialize(active)` to a `std::string` + builds the runtime systems (`ScriptSystem`, `Physics2DSystem` with `setScriptSystem`/gravity, `SpriteAnimationSystem`); `pause()/resume()`; `stop()` clears the world + `YAMLDeserializer::deserialize`s the snapshot back + tears the systems down.
- [x] Loop integration: while Playing, `EditorApplication` runs `m_play.update` (physics + scripts + animation — the GL-free sim); the `ViewportPanel` keeps rendering and steps **particles** with the real dt (they couple sim+render inside `beginScene`), marking `WorldTransform`s dirty so physics moves show. Paused freezes the sim but still renders. Menu-bar toolbar Play/Pause-Resume/Stop + a `[EDIT]/[PLAYING]/[PAUSED]` readout; `Ctrl+P` toggles.
- [x] Play-mode input: the editor owns an `InputManager` (`init` makes it the active manager for the `Input::` facade) updated every frame, so game scripts read input during play; the viewport's editor-camera pan/zoom is disabled while playing (`if (!playing)`).
- [x] History/dirty: during play `ctx.history` is set to `nullptr`, so panel edits mutate the live scene directly **without** touching the Edit-mode undo stack (and are discarded by Stop's snapshot restore) — the edit history stays intact across a play→stop cycle. Selection is cleared on Stop.

**Verify Phase 6:** ✅ `PlayModeController.cpp` compiles `-Werror` against clean headers; it reuses the `serialize`/`deserialize` round-trip (covered by Epic 05/06 tests) for snapshot/restore. A standalone TST-03 (serialize → mutate → restore == pre-play) lands in Phase 9; live play→stop with scripts/physics is the user build.

---

## Phase 7 — Entity picking (RHI ID attachment + CPU fallback)

Implements `VP-04, VP-06`, `NFR-02`.

- [x] **CPU picking** (`editor/Picking.h/.cpp`, ImGui-free): `Picking::pick(World&, viewProjection, viewportSize, mousePx)` unprojects the panel-local cursor via `screenToWorld` (top-left origin) and tests each `WorldTransform`+`SpriteRenderer` entity's world-space AABB (center = `wt.matrix[3]`, half = basis-vector length·0.5); front-most by `SpriteRenderer.layer` wins, ties to the later entity; `NULL_ENTITY` on a miss.
- [x] `ViewportPanel`: a left-click (no drag) maps `GetMousePos − GetItemRectMin` to viewport-local pixels and calls `Picking::pick`, setting `ctx.selected` (a miss clears it). Disabled while playing. Accounts for panel offset + the image size (= FBO size), so it's correct at any panel size (VP-06).
- [~] **GPU ID-attachment path deferred** (the plan's contingency, reversed in priority): the `FramebufferSpec.idAttachment` + `IFramebuffer::readPixelInt` + `Renderer2D` MRT shader changes are an engine-side GL change that can't be exercised headless, so v1 ships the CPU path behind the same `pick` interface; the GPU path can be added later without touching call sites. (Tradeoff: AABB picking ignores rotation/precise sprite alpha — fine for v1.)

**Verify Phase 7:** ✅ `Picking.cpp` compiles `-Wall -Wextra -Wpedantic -Werror` against clean headers; the AABB test + `screenToWorld` mapping are simple and deterministic. A Catch2 `TestPicking` (center hit, off-center hit at a non-default viewport size, overlapping front-most, empty miss) lands in Phase 9 and runs in the user build (entt-heavy, exceeds the sandbox compile budget).

---

## Phase 8 — Asset Browser panel

Implements `AB-01 … AB-05`.

- [x] `panels/AssetBrowserPanel.h/.cpp`: left dir-tree of `assets/` (recursive `std::filesystem`, click to select), right list of the selected dir's files (sorted), current-path label.
- [~] Thumbnails deferred: files show their name (no live `ImGui::Image` thumbnails in v1 — that needs per-file `AssetManager` texture loads + GL, untestable headless and a perf/caching concern). Type icons/thumbnails are a polish follow-up.
- [x] Drag source: each file emits an `"EMBER_ASSET_PATH"` payload (full path). The Inspector's **SpriteRenderer** `texture` field is a drop target that assigns `texturePath` + clears the resolved handle so it reloads. Double-clicking a `.escene` calls the open-scene callback (wired to `EditorApplication::openSceneFile`).
- [x] Right-click ▸ Delete (with a confirm modal, `std::filesystem::remove`). Rename and Show-in-Explorer deferred (the latter needs per-OS shell calls); the tree re-scans the filesystem every frame so external changes show up without a manual refresh.

**Verify Phase 8:** ✅ logic is standard `std::filesystem` + the verified `EditorContext`/`AssetHandle` APIs; `EditorContext.h` compiles `-Werror`. Browsing, drag-to-assign (sprite updates), and open-from-grid run in the user build (ImGui TU). The drag-assign acceptance item (AB-03) is wired end-to-end.

---

## Phase 9 — Tests, header hygiene, final verification

Implements `TST-01 … TST-06`, `NFR-01/03/05/06`.

- [x] `tests/editor/` wired into `ember_tests` (linking the new `ember_editor_core`): `TestCommandHistory.cpp` (TST-01/02 — push/undo/redo, redo-tail truncation, cap eviction, `pushExecuted`), `TestSceneCommands.cpp` (TST-02 — create/rename/reparent undo-to-prior-state + SceneOps cycle/self guards; the prefab-restore path covers TST-03's snapshot mechanism), `TestPicking.cpp` (TST-05 — center/off-center hits, front-most on overlap, non-square viewport, empty miss), `TestInspectorModel.cpp` (TST-04 recording visitor + TST-06 add-component filtering + snapshot/restore). All ImGui-free.
- [x] Header hygiene: ImGui is in **no** `ember_*` header and no `ember_editor_core` header — the inspect/command/picking/scene-ops logic is ImGui-free (proven by it linking into `ember_tests`). The deferred GPU-picking RHI change is the only engine-touching item and was not made, so engine libs are untouched by this epic.
- [x] Editor split into `ember_editor_core` (ImGui-free static lib, always built) + `ember_editor` (ImGui exe, GLFW/ImGui-guarded). `-Wall -Wextra -Wpedantic -Werror` clean on both MinGW (Win) and clang (macOS); ImGui third-party TUs warnings-allowed.
- [~] User build: editor compiles & runs on Windows + macOS; exit-criteria (create/select/inspect/edit/undo, save/reload, play/stop, viewport pick, drag-assign) are exercisable in-app. Full `ctest`/`sanitize` green is the user's run; all four editor test TUs compile `-Werror` against clean headers in the sandbox (CommandHistory + Picking also verified by standalone drivers earlier).

### Acceptance checklist (from `02-requirements.md` §14) — all items map to phases 0-9.

---

## Phase ordering & dependencies

- **Critical path to a usable editor:** 0 → 1 → 2 → 3 → 4 → 5 (open, inspect, edit, undo, save). 
- **6 (Play/Stop)** depends on 5's serialization wiring. **7 (Picking)** is independent of 6 and can slot anywhere after 1 (CPU fallback) with the GL path whenever the RHI change lands. **8 (Asset Browser)** depends on 3/4 for drag-assign commands. **9** closes out.
- Each phase ends buildable; engine libraries stay ImGui-free throughout, so the only cross-cutting engine change is the Phase 7 RHI framebuffer ID attachment.
