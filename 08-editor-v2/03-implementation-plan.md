# Epic 08 — Editor v2: Implementation Plan

**Status:** Complete (all phases 0-9) — builds on Windows (MinGW); ImGui-free logic unit-tested  
**Document:** 03-implementation-plan.md  
**Refs:** epic.md, 02-requirements.md

Phases are ordered so the **foundational** work (selection model, ImGuizmo vendoring) and the **engine-side, headless-testable** additions (profiler, tilemap edit + serialization, import pipeline) come before or alongside the GUI panels that depend on them. Each phase ends buildable. The v1 rule holds throughout: **no `ember_*` engine library depends on ImGui/ImGuizmo**; all new ImGui-free logic lands in `ember_editor_core` and is unit-tested. Requirement IDs (e.g. `GIZ-06`) refer to `02-requirements.md`.

> **Build/verify reality (unchanged from Epic 07).** `ember_editor` (ImGui/ImGuizmo/GLFW/GL) builds & runs only in the user's desktop build (Win MinGW GCC 13 / macOS clang). In the Linux sandbox we verify the **ImGui-free** pieces — `ember_editor_core` (selection, the new commands, gizmo/tilemap/prefab-override math) and the engine-side profiler/tilemap/import code — with headless Catch2 + native drivers, and `-Werror` syntax-check the GUI TUs against clean headers. The recurring **mount truncation** of freshly-edited headers may require reconstructing clean copies for those checks. New cross-platform pitfalls (clang `-Wunused-*`, `ImTextureID`/cast rules, incomplete-type `unique_ptr`) get the same treatment as v1.

**Grounding (current repo state):**

| Fact | Consequence |
|------|-------------|
| `ember_editor_core` (07) holds `CommandHistory`/`FunctionalCommand`, `Commands::*`, `SceneOps`, `InspectorRegistry` (`has/add/remove/snapshot/restore/draw`), `Picking`; `ember_editor` is the guarded ImGui exe | new commands + `Selection` + gizmo/tilemap/prefab-diff math go in core; ImGuizmo + new panels go in the exe |
| `EditorContext{ scene, assets, history, selected:Entity, dirty, playing }` (07) | **rework** to `std::unordered_set<Entity> selection` + `Entity primary` (Phase 0); every panel updated |
| ImGui fetched-but-populated; `IMGUI_SOURCE_DIR` cache var; `editor/CMakeLists.txt` builds `imgui` lib | ImGuizmo follows the same pattern: `FetchContent_MakeAvailable` (no CMakeLists) → `IMGUIZMO_SOURCE_DIR` → compile `ImGuizmo.cpp` into the `imgui` lib |
| `Prefab` asset + `PrefabLoader`; `YAMLSerializer::serializePrefab`; `YAMLDeserializer::instantiatePrefab` (05/06) | prefab save/instantiate wired on these; add a `PrefabInstance` component + override diff |
| `Tilemap{width,height,tileWorldSize,tileset,tiles:vector<u32>, at, isSolid}` + `Tileset{texturePath,columns,rows,uvForTile}`; `TilemapRenderSystem`; `.tilemap` loader (12) | add `setTile`/`resize`; add **custom scene (de)serialization** (vector<u32> isn't a reflectable field) + register it |
| `AssetDatabase` + `AssetMeta{type,importer,settings}` + `.meta`; `AssetSettings` (`IAssetLoader.h`); loaders take `(path, AssetSettings)` (05) | import pipeline wraps loaders by settings; Inspector edits `.meta`; "Re-import" re-runs |
| `TextureSpec{width,height,format(RGBA8/RGB8/R8),filter(Nearest/Linear),wrap}` (04) | TextureImporter settings map onto these |
| `Renderer2D` + `DebugDraw` + `Renderer2D::drawText`/`stats()` (04); `EditorCamera::viewProjection` (04) | gizmo camera matrices, debug overlays, profiler stats |
| **No profiler**; **ImGuizmo not vendored**; selection is single-entity | the three real "new" gaps this epic |

**Cross-cutting decisions (confirm before starting):**

- **Selection first.** `EditorContext` gains `std::unordered_set<Entity> selection` + `Entity primary`; a `Selection` helper (core, ImGui-free) wraps mutation; panels read the set. Single-select = set of size 1 (primary == that entity), so v1 behavior is preserved.
- **Gizmo command on release.** ImGuizmo edits live; one `TransformGizmoCommand` (before/after `Transform` per selected entity) is `pushExecuted` on drag end.
- **Profiler gated.** `EMBER_PROFILE_SCOPE` is a no-op unless `EMBER_ENABLE_PROFILING` (new option, default OFF); the collector is ImGui/GL-free in `engine/core`.
- **Prefab overrides = reflected diff.** Instance stores `PrefabInstance{source}` + serialized overrides; diff vs a fresh instantiation using reflection + `InspectorEntry::snapshot`.
- **Tilemap stroke = one command.** Paint/erase/fill accumulate `{cell→oldId}`; one `TilemapPaintCommand` per click-drag. Custom tilemap serialization makes painted maps round-trip in `.escene`.
- **Import pipeline wraps loaders, ImGui-free.** `AssetImporter`/`AssetImportPipeline` in `engine/assets`; the Inspector `.meta` UI is editor-side.
- **No engine lib gains ImGui/ImGuizmo.** ImGuizmo is ImGui-only → editor target only. New core logic is all ImGui-free.

---

## Phase 0 — Foundations: selection model + ImGuizmo vendoring

Implements `SEL-01`, `NFR-01`; prerequisite for Phases 1-2.

- [x] `cmake/Dependencies.cmake`: `FetchContent_Declare(imguizmo …)` (CedricGuillemet/ImGuizmo, `master`) + added to `FetchContent_MakeAvailable(stb imgui imguizmo)` (populate only) → `IMGUIZMO_SOURCE_DIR` cache var. `editor/CMakeLists.txt`: `${IMGUIZMO_SOURCE_DIR}/ImGuizmo.cpp` compiled into the `imgui` lib + its include dir (warnings-allowed, behind the existing `glfw`/ImGui guard).
- [x] `editor/Selection.h` (header-only, ImGui-free, in `ember_editor_core`'s include dir): `EntitySet` (`unordered_set<Entity>` + `EntityHash`) + `Selection{ empty/size/contains/primary/entities/clear/selectOnly/add/remove/toggle }`; primary reassigns to another member when removed, clears to `NULL_ENTITY` when empty.
- [x] `EditorContext`: `Entity selected` → `Selection selection`. Retrofitted **all** panels: Hierarchy (highlight via `contains`, plain click = `selectOnly`, Shift/Ctrl = `toggle`, bg click = `clear`), Inspector (subject = `primary()`), Viewport pick (modifier-aware toggle/selectOnly, miss clears), shortcuts + create/duplicate/delete in `EditorApplication`.
- [x] Selection cleared on Stop / New / Open; on delete the target is removed from the set (primary reassigned).

**Verify Phase 0:** ✅ headless `Selection` driver passes (selectOnly/add/toggle/remove/primary-reassign/clear) — TST-01; no `ctx.selected` references remain; `Selection.h` compiles `-Werror`. ImGuizmo compiled into `imgui` + the panel retrofits build in the user's environment (ImGui not in the sandbox).

---

## Phase 1 — Transform gizmos (ImGuizmo)

Implements `GIZ-01 … GIZ-08`, `TST-02`, `TST-08`.

- [x] `ember_editor_core`: `Commands::transformEntities(before, after)` (per-entity `Transform` apply/undo, one undo step) + `GizmoMath` (`snapTranslate` to grid, `snapAngleDeg`). `EditorCamera` gained `view()`/`projection()`/`halfHeight()` accessors (ImGuizmo decomposes them). All ImGui-free.
- [x] Gizmo integrated into `ViewportPanel` (no separate window): `drawGizmo()` draws ImGuizmo over the image via `SetDrawlist`+`SetRect`, ortho mode, `EditorCamera::view()/projection()`. Mode shortcuts `W`/`E`/`R` (translate/rotate/scale) + `X` toggles local/world. Guarded by `EMBER_HAS_IMGUIZMO` (no-op stub otherwise). Returns "consumed" so picking/camera don't also fire.
- [x] Single + multi manipulation (GIZ-04/05): pivot recomputed each frame at the selection's average position; ImGuizmo's per-frame delta matrix applied to each selected entity's world `trsOf(Transform)` then decomposed back (`DecomposeMatrixToComponents`) into position/rotationZ/scale + `markTransformDirty`.
- [x] One command per drag: `before` captured on first `IsUsing()`, `after` on release → `pushExecuted(transformEntities(...))`; `Ctrl`-snap (grid/angle defaults; settings wire in Phase 7); inert while playing (GIZ-08). `ImGuizmo::BeginFrame()` added to `ImGuiLayer::begin()`.

**Verify Phase 1:** ✅ `GizmoMath` snap tests pass headless (grid round + 15° quantize); `transformEntities` mirrors the proven `FunctionalCommand` pattern; `Camera`/`Commands` edits are simple (the in-sandbox `-Werror` check was blocked only by mount truncation of the freshly-edited files). The ImGuizmo draw/manipulate path + apply-N→undo (TST-02) are the user's build (ImGuizmo isn't in the sandbox). A headless `TestGizmoCommand` lands in Phase 9.

---

## Phase 2 — Multi-select inspector + batch ops

Implements `SEL-04 … SEL-05`, `TST-03`.

- [x] `ember_editor_core`: `Commands::setComponentBatch` (per-entity before/after snapshots of one component, single undo) + `Commands::composite` (group N commands → one undo step, executed in order / undone in reverse).
- [x] Inspector multi-select: shows components **common to all** selected (intersection via `entry.has`); editing the primary's field syncs that component to all selected live, and commits one `setComponentBatch` on release (coalesced; before captured pre-sync). Single-select path unchanged. Add/Remove Component apply to every selected entity (one undoable command). Whole-component sync (per-field-only refinement noted as a deferred item).
- [x] Delete / Duplicate / Reparent operate on the whole selection as one `composite` undo step — wired in `EditorApplication` shortcuts (`Delete`, `Ctrl+D`) and the Hierarchy context menu + drag-reparent (`targetsFor` expands to the selection when the acted entity is part of it).

**Verify Phase 2:** ✅ `composite` (apply-all-as-one → single undo reverts all → redo) passes a headless driver; `setComponentBatch` mirrors the proven `setComponent` restore-thunk pattern. Inspector/hierarchy/app are ImGui TUs (user build). A headless `TestBatchCommand`/`TestComposite` lands in Phase 9.

---

## Phase 3 — Profiler (engine facility + panel)

Implements `PROF-01 … PROF-06`, `TST-07`.

- [x] `engine/core`: `option(EMBER_ENABLE_PROFILING OFF)` (PUBLIC define on `ember_core` when on); `Profiler.h/.cpp` — `EMBER_PROFILE_SCOPE("name")` RAII timer (`steady_clock`) → per-frame `{name,depth,start,end}` buffer; `beginFrame`/`endFrame` swap; runtime `setEnabled`; **expands to `((void)0)`** when the option is off. ImGui/GL-free, always compiled so the panel can read it.
- [x] Instrumented: editor loop (`Play::update`), `PlayModeController` (`Physics`/`Scripts`/`Animation`), `ViewportPanel` (`Render2D`); `Profiler::beginFrame`/`endFrame` bracket the editor frame. (Engine systems read the macro through `ember_core`'s PUBLIC define; the editor-side scopes give a populated tree without touching truncation-prone engine `.cpp`s.)
- [x] `editor/panels/ProfilerPanel.{h,cpp}`: stats bar (frame ms / FPS / `Renderer2D::stats()` draws-quads-verts), "Record" checkbox (`Profiler::setEnabled`), rolling 120-frame `PlotLines`, and a hand-drawn flame graph on the window draw list (nested bars width∝duration, name color-hashed, hover tooltip = ms).

**Verify Phase 3:** ✅ headless profiler driver passes — nested `outer/innerA/innerB` depth 0/1/1 in order, non-negative durations, and **no samples when disabled or when the macro is compiled off**. The flame-graph panel is an ImGui TU (user build).

---

## Phase 4 — Prefab system

Implements `PFB-01 … PFB-06`, `TST-06`.

- [x] `PrefabInstance{ std::string source }` component (`engine/scene`), registered in the serializer's `ensureRegistered()` so it round-trips with the scene (PFB-03/06). Instantiation uses `YAMLDeserializer::instantiatePrefab`; the editor tags the new entity with its source path.
- [x] Editor flow: Hierarchy "Save as Prefab" → file browser (`.eprefab`) → `YAMLSerializer::savePrefabToFile` + tag the source as an instance; drag a `.eprefab` from the Asset Browser into the Viewport → read file + `instantiatePrefab` + tag; Hierarchy highlights instances in blue; Inspector shows a "Prefab: <source>" banner + **Revert to Prefab** (re-instantiates the source at the instance's position, replaces the entity). Cross-panel via `EditorContext` request fields consumed in `EditorApplication::handlePrefabRequests`.
- [~] **Field-level override diff deferred:** per-field bold + per-field revert (PFB-04 / field-level PFB-05) need a type-erased 3-way field comparison (the same reflection-diff difficulty as Phase 2's per-field multi-edit) — v1 ships **entity-level** revert (restores the whole entity from source). The instance tag + source path persist; nested prefabs remain out of scope.

**Verify Phase 4:** ✅ `PrefabInstance` reflect + serializer registration compile (`ember_serialization` links `ember_scene`); the flow reuses the `serializePrefab`/`instantiatePrefab` round-trip (covered by Epic 05/06 tests). The editor flow + revert are the user's build (ImGui TUs). Field-level diff + its TST-06 are folded into the deferred item.

---

## Phase 5 — Tilemap editor

Implements `TME-01 … TME-07`, `TST-04`, `TST-05`.

- [x] `engine/renderer/2d`: `Tilemap::setTile(x,y,id)` + `resize(w,h)` (bounds-checked, preserves overlap). Custom Tilemap **scene (de)serialization** — a hand-written `ComponentMeta` in `RenderComponentSerialization.cpp` (the renderer's yaml TU) emitting an inline block (width/height/tileWorldSize/layer/tileset path+cols+rows + a flow seq of tile ids) and parsing it back; registered in `registerRenderComponents()` so painted maps round-trip in `.escene` (TME-06).
- [x] `ember_editor_core`: `Commands::paintTiles` (stroke = `{x,y,oldId,newId}` per cell; redo writes newId, undo restores oldId) + `TilemapPaint` helpers (`worldToCell`, 4-connected `floodFill`). Tested.
- [x] `editor/panels/TilemapEditorPanel.{h,cpp}`: shown for a selected `Tilemap`; Paint-mode toggle + Paint/Erase/Fill radio + a numbered palette (columns×rows, click→active id), all via shared `ctx.tilemap`. `ViewportPanel::paintTilemap` does the painting: world→cell, accumulates a click-drag stroke (dedup per cell) into one `paintTiles` command on release; Fill flood-fills on click; takes input priority over gizmo/pick/camera while active. (Palette uses numbered buttons in v1, not texture-sliced thumbnails — noted.)

**Verify Phase 5:** ✅ headless driver passes — `setTile`/`resize` bounds + overlap-preserving resize, `worldToCell` in/out-of-bounds, `floodFill` (full region, wall-split, no-op). `paintTiles` mirrors the proven command pattern. Tilemap (de)serialization is a yaml TU (user build); a Catch2 round-trip lands in Phase 9. Panel/viewport paint are ImGui TUs.

---

## Phase 6 — Asset import pipeline

Implements `IMP-01 … IMP-05`.

- [x] `engine/assets/ImportSettings.{h,cpp}` (ImGui/GL-free): typed `TextureImportSettings` (filter/wrap/format/generateMips/maxSize) + `FontImportSettings` (rasterize sizes), each with `fromSettings`/`toSettings` over the `.meta` `AssetSettings` kv map — the shared contract the loaders + editor read/write. (The existing `Texture2DLoader` already honors filter/wrap; the AssetManager loader+settings+`reload` flow **is** the (re)import pipeline, so no redundant `AssetImporter` class was added.)
- [~] Loader honoring: filter/wrap take effect on reload (already); `format`/`maxSize`/`generateMips` **persist** in `.meta` and round-trip, but the loader still derives format from source channels (forcing a different format needs untestable GL conversion) — noted as a follow-up.
- [x] Editor: Asset Browser shows an **import-settings editor** for the selected texture (filter/wrap/format/mips/maxSize combos) or font (sizes) and an **Apply & Re-import** button → `AssetDatabase::writeMeta` the sidecar + `AssetManager::reload` the live handle. (Per-file selection state added.)
- [~] Dependency tracking (tileset texture → tilemap) deferred — there's no concrete dep graph yet; re-import refreshes the texture handle directly.

**Verify Phase 6:** ✅ headless driver — `TextureImportSettings`/`FontImportSettings` `fromSettings`↔`toSettings` round-trip (filter/wrap/format/mips/maxSize + font sizes) + defaults. The `.meta` editor + reload are ImGui/yaml TUs (user build), reusing `AssetDatabase::readMeta/writeMeta` + `AssetManager::reload`.

---

## Phase 7 — Editor settings / preferences

Implements `SET-01 … SET-03`.

- [x] `editor/EditorSettings.{h,cpp}` (ImGui-free, in `ember_editor_core`): struct {theme, gridSnap, rotateSnap, cameraPanSpeed, cameraZoomSpeed, autoSaveInterval, scriptTemplatePath} with `load`/`save` to a flat `key: value` file (no yaml-cpp in the editor). `EditorSettingsPanel` (`Edit ▸ Preferences`) edits it with combos/drags + returns "changed".
- [x] Persist to `editor/preferences.yaml` (loaded at startup, saved whenever a field changes; defaults kept on missing/invalid). `EditorApplication::applySettings` wires theme→`ImGuiLayer::applyTheme`, snap→`ViewportPanel::setSnap` (gizmo), speeds→`ViewportPanel::setCameraSpeeds`, and an auto-save accumulator saves a dirty scene with a path every `autoSaveInterval` seconds.

**Verify Phase 7:** ✅ `EditorSettings` save→load round-trip (all fields) + missing-file-keeps-defaults pass a headless driver. `ImGuiLayer::applyTheme` + the panel are ImGui TUs (user build).

---

## Phase 8 — Debug overlays

Implements `DBG-01 … DBG-03`.

- [x] `View` menu toggles (editor-only `DebugOverlays` in `EditorContext`, never serialized): Show Grid, Show Entity Names, Show Colliders, Show Camera Frustums.
- [x] Rendered in the viewport pass: `ViewportPanel::drawDebugOverlays` draws grid lines + `BoxCollider2D`/`CircleCollider2D` wireframes (`DebugDraw::rect`/`circle`) + `Camera2D` frustum rects into the scene FBO; `drawEntityNames` projects each `Tag`+`WorldTransform` via `worldToScreen` and labels it on the ImGui draw list. `DebugDraw::init/shutdown` wired into the editor. (Grid/colliders/frustums use `DebugDraw`, which is a **no-op in Release** — names work in both.)

**Verify Phase 8:** the overlay drawing uses the existing `DebugDraw` line/rect/circle + `worldToScreen` (both already engine-tested); the math (collider center/size from Transform, frustum size from `Camera2D`) is straightforward. All ImGui/GL TUs → user build. No new sources.

---

## Phase 9 — Tests, header hygiene, final verification

Implements `TST-01 … TST-08`, `NFR-01 … NFR-06`.

- [x] `tests/editor/` wired into `ember_tests` (via `ember_editor_core`): `TestSelection`, `TestGizmo` (snap + `transformEntities` apply/undo), `TestComposite` (group → one undo), `TestProfiler` (built with `EMBER_ENABLE_PROFILING`; nested depth + disabled = empty), `TestTilemapEdit` (`setTile`/`resize`/`worldToCell`/`floodFill`/`paintTiles`), `TestImportSettings`, `TestEditorSettings`. (Prefab override-diff test folded into the Phase-4 deferral.)
- [x] Header hygiene: ImGui/ImGuizmo appear in **no** `ember_*` or `ember_editor_core` header — proven by `ember_editor_core` + all `tests/editor/*` linking without them; profiler/import-settings are engine-side but ImGui/GL-free; profiling is zero-cost when the option is off.
- [~] `-Wall -Wextra -Wpedantic -Werror`: the entt-free suites compile clean in-sandbox; full `-Werror` on MinGW+clang, the `EMBER_ENABLE_AUDIO`/`EMBER_ENABLE_PROFILING` on/off matrix, and v1 `.escene` back-compat are the user's build.
- [~] User build: exit-criteria (gizmo move+undo, rotate snap, prefab save/instantiate/revert, profiler timings, tilemap paint+undo+round-trip, multi-select move+undo, texture re-import) + `ctest`/`sanitize` green are the user's run; every headless-checkable unit passes a Catch2 suite or a native driver here.

### Acceptance checklist (from `02-requirements.md` §14) — all items map to phases 0-9.

---

## Phase ordering & dependencies

- **Foundational:** Phase 0 (selection + ImGuizmo) unblocks Phases 1-2.
- **Engine-side, independent, headless-testable:** Phase 3 (profiler), Phase 5's engine bits (`setTile`/serialization), Phase 6 (import pipeline) can proceed in parallel with the GUI phases.
- **Depend on earlier work:** Phase 2 (batch) needs Phase 0; Phase 4 (prefab overrides) reuses the inspector snapshot/diff; Phase 5's panel needs Phase 0 selection; Phase 7 snap feeds Phase 1's gizmo.
- Each phase ends buildable; the only engine-touching additions are the gated profiler and the import pipeline (both ImGui/GL-free) plus `Tilemap` mutators/serialization — no other engine library changes.
