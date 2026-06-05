# Epic 08 — Editor v2: Requirements

**Status:** Draft  
**Epic:** 08-editor-v2  
**Depends on:** 07-editor-v1, 05-input-and-assets, 12-2d-gameplay-and-audio  
**Blocks:** 09-3d-foundation (the gizmo system must be 3D-ready)

---

## 1. Overview

Take the v1 editor from "functional" to "a tool you'd ship a game with": on-screen **transform gizmos**, **multi-selection**, a **prefab** workflow, a **profiler** panel, an in-viewport **tilemap painter**, an **asset import pipeline** with per-asset settings, **editor preferences**, and **debug overlays**.

Everything is built on the Epic 07 foundation and keeps its core rule: **no `ember_*` engine library depends on ImGui.** New editor logic that is ImGui-free (commands, selection model, gizmo math, tilemap-paint math, prefab-override diffing) lives in `ember_editor_core` so it stays unit-testable headless; ImGui panels/widgets stay in the `ember_editor` executable. The only genuinely engine-side additions this epic are a lightweight **profiling facility** (`engine/core`) and an **asset import pipeline** (`engine/assets`); both are GL/ImGui-free.

This epic adds **two new third-party dependencies**: ImGuizmo (gizmos) — vendored, ImGui-only — and nothing else (the profiler flame graph is hand-rolled on ImGui's draw list).

### 1.1 Grounding in the current codebase

| Existing facility (epic) | How v2 uses it / what's missing |
|--------------------------|---------------------------------|
| `ember_editor_core` (07): `CommandHistory` + `FunctionalCommand`, `Commands::{create,remove,duplicate,reparent,rename,setComponent}`, `SceneOps`, `InspectorRegistry` (`has/add/remove/snapshot/restore/draw`), `Picking::pick` | gizmo/batch/paint/prefab commands extend this; multi-select reworks selection; the inspector registry's snapshot/restore powers batch edits + override diffing |
| `EditorContext{ scene, assets, history, selected (single Entity), dirty, playing }` (07) | **changed:** selection becomes a `std::unordered_set<Entity>` (+ a "primary"/active entity); every panel updated (SEL-01) |
| `ViewportPanel` renders scene→FBO→`ImGui::Image`, owns `EditorCamera`, does CPU click-pick (07) | gizmos draw over the image via ImGuizmo using `camera.viewProjection()`; tilemap paint is a new viewport input mode |
| `Renderer2D` + `DebugDraw` (04) | debug overlays (grid, names, collider/frustum wireframes) draw through `DebugDraw` / `Renderer2D` in the viewport pass |
| `Prefab` asset + `PrefabLoader`, `YAMLSerializer::serializePrefab(scene,root)`, `YAMLDeserializer::instantiatePrefab(scene,yaml,pos)` (05/06) | the prefab workflow is wired on these; `.eprefab` is the on-disk form; "Save as Prefab" = `serializePrefab` → file; drag-in = load + `instantiatePrefab` |
| `Tilemap{ width,height,tileWorldSize,tileset,tiles(vector<u32>), at(x,y), isSolid }`, `Tileset{ texturePath,columns,rows,uvForTile }` (12); `TilemapRenderSystem`, `.tilemap` loader | painting needs a **new** `setTile(x,y,id)` + resize; **Tilemap is not yet reflected/registered** with the scene serializer (its `vector<u32>` isn't a visitor type) → v2 adds custom tilemap (de)serialization (TME-06) |
| `AssetDatabase` + `AssetMeta{ importer, settings }` + `.meta` sidecars, `AssetSettings`, loaders (Texture2D/Font/…) (05) | the import pipeline formalizes "source file + `.meta` settings → engine asset"; the Inspector edits `.meta` settings; **no `AssetImporter` exists yet** (IMP) |
| `ImGuiLayer` (07): `StyleColorsDark`, `IniFilename = editor/layout.ini` | preferences add theme switching + a `preferences.yaml`; layout.ini unchanged |
| **No profiler exists** — there is no `EMBER_PROFILE_SCOPE` or timing collector | v2 adds a minimal `engine/core` profiler (PROF-01) the panel visualizes |
| **ImGuizmo not vendored** — `third_party/` has glad/glfw_stub/spdlog_stub/tl only | v2 vendors ImGuizmo (NFR-01) |

### 1.2 Key design decisions

- **Selection model rework (foundational, do first).** `EditorContext.selected` (a single `Entity`) becomes `std::unordered_set<Entity> selection` plus `Entity primary` (the gizmo pivot / inspector subject for "active" semantics). A tiny `Selection` helper in `ember_editor_core` (add/remove/toggle/clear/contains/primary) keeps panels simple and is unit-testable. All v1 panels are updated to read the set.
- **Gizmo = ImGuizmo, command on release.** ImGuizmo is drawn in the viewport; while dragging, the entity transform updates live; on release a single `TransformGizmoCommand` (before/after transforms for all selected) is pushed (one undo per drag), mirroring the v1 inspector-drag coalescing.
- **Profiler is engine-light + editor-visual.** `engine/core` gets `EMBER_PROFILE_SCOPE("name")` (RAII; compiles to nothing unless `EMBER_ENABLE_PROFILING`) feeding a per-frame ring buffer of `{name, depth, start, end}`; the `ProfilerPanel` renders a hand-drawn flame graph + a rolling frame-time graph from that buffer. No third-party profiler.
- **Prefab overrides = serialized diff.** A prefab instance stores its source path + a list of per-field overrides (`{componentName, fieldName} → value`) captured by comparing the instance to a freshly-instantiated source (reusing `InspectorEntry::snapshot` + reflection). Overrides serialize alongside the entity; the Inspector bolds overridden fields and offers "Revert to Prefab". **No nested prefabs** this epic.
- **Tilemap edits are commands at stroke granularity.** A paint/erase/fill *stroke* (one click-drag) captures the set of `{cell → oldId}` it changed and pushes one `TilemapPaintCommand`. Painting is a viewport input *mode* sharing the viewport (no separate window), mutually exclusive with camera nav.
- **Import pipeline wraps existing loaders.** `AssetImporter` reads `.meta` settings and produces the engine asset via the existing loaders; the Inspector edits those settings and "Re-import" re-runs. Dependency tracking marks dependents dirty (tileset texture → tilemap).
- **No engine library depends on ImGui** (unchanged from v1). ImGuizmo is ImGui-only and lives with `ember_editor`. `ember_editor_core` gains: `Selection`, gizmo math helpers, `TransformGizmoCommand`, `BatchSetFieldCommand`, `TilemapPaintCommand`, prefab-override diff — all ImGui-free.

---

## 2. Definitions

- **Gizmo** — the on-screen translate/rotate/scale manipulator (ImGuizmo).
- **Selection / primary** — the set of selected entities; the *primary* is the active one (gizmo pivot, single-edit subject).
- **Prefab** — a `.eprefab` YAML file holding one entity sub-tree; an **instance** is an entity created from it that remembers its source + overrides.
- **Override** — an instance field whose value differs from the prefab source.
- **Stroke** — one paint/erase/fill click-drag interaction; the unit of tilemap undo.
- **Importer** — code that turns a source file + `.meta` settings into an engine asset.
- **Scope sample** — one `EMBER_PROFILE_SCOPE` timing record for a frame.

---

## 3. Transform gizmos (`editor/gizmos/`, ImGuizmo)

- **GIZ-01** Integrate ImGuizmo in the Viewport: translate / rotate / scale manipulators drawn over the scene image using the `EditorCamera` view + projection (decomposed for ImGuizmo's row-major API).
- **GIZ-02** Mode shortcuts (when the viewport is focused and no text field is active): `W`/`T` translate, `E`/`R` rotate, `S` scale (final binding chosen to not clash with existing `Ctrl+S`/`F`); a toolbar shows the active mode.
- **GIZ-03** Local vs. world space toggle (toolbar + a shortcut); rotate/scale respect it.
- **GIZ-04** Single-select: the gizmo edits the entity's `Transform` (its `WorldTransform` is rebuilt by `TransformSystem`). Parented entities edit correctly (gizmo works in world space, result written back to local transform).
- **GIZ-05** Multi-select: the gizmo sits at the selection's bounding center; each selected entity transforms relative to its own pivot by the same delta.
- **GIZ-06** A drag produces **one** `TransformGizmoCommand` capturing before/after `Transform` for every affected entity (single undo for the whole drag), pushed on release via `pushExecuted` (live edit already applied).
- **GIZ-07** Snapping: holding `Ctrl` snaps translate to a configurable grid (default 1 unit) and rotate to 15° increments (snap values come from Editor Settings, §8).
- **GIZ-08** The gizmo only captures input when hovered/active; it never fires while the camera is panning or in Play mode.

---

## 4. Multi-selection (`ember_editor_core` + panels)

- **SEL-01** `EditorContext` selection becomes `std::unordered_set<Entity> selection` + `Entity primary`. A `Selection` helper (ImGui-free) provides `add/remove/toggle/clear/contains/primary/empty/size`.
- **SEL-02** Hierarchy + Viewport picking: plain click = select-only (clear others, set primary); `Shift`/`Ctrl`+click = toggle membership (update primary).
- **SEL-03** The Hierarchy highlights **all** selected rows; the Viewport pick respects modifiers.
- **SEL-04** Inspector in multi-select shows components **common to all** selected entities; editing a shared field applies to all via one `BatchSetFieldCommand` (before/after per entity, single undo). Single-select behaves exactly as v1.
- **SEL-05** Delete / Duplicate / Reparent operate on the entire selection (each as one undo step, e.g. a composite command or a batch).
- **SEL-06** Selection is cleared appropriately when entities are deleted or on Stop; the primary stays valid (or resets to any remaining member / `NULL_ENTITY`).

---

## 5. Prefab system

- **PFB-01** "Save as Prefab" (Hierarchy right-click): `YAMLSerializer::serializePrefab(scene, entity)` → write a `.eprefab` file under `assets/` (path chosen via the file browser); the source entity optionally becomes an instance of it.
- **PFB-02** Instantiate: dragging a `.eprefab` from the Asset Browser into the Viewport or Hierarchy loads it and calls `YAMLDeserializer::instantiatePrefab` (at the drop world-position for the Viewport, else origin). `Scene::instantiate(AssetHandle<Prefab>, pos)` is wired as the convenience entry point.
- **PFB-03** A prefab **instance** records its source prefab path (a component, e.g. `PrefabInstance{ std::string source; }`) so the editor can show/diff it; instances are marked in the Hierarchy (distinct icon/color).
- **PFB-04** **Overrides:** the editor computes per-field differences between the instance and a fresh instantiation of its source (reusing reflection + `InspectorEntry::snapshot`); overridden fields render **bold** in the Inspector.
- **PFB-05** Right-click an overridden field → "Revert to Prefab" restores the source value (a `setComponent`/field command, undoable).
- **PFB-06** Overrides persist: an instance serializes its source path + its override set in the scene file; on load the entity is rebuilt from the source then overrides are re-applied. **No nested prefabs** (PFB instances inside a prefab) this epic.

---

## 6. Profiler (`engine/core` profiling + `editor/panels/ProfilerPanel`)

- **PROF-01** Engine profiling facility (`engine/core`, ImGui/GL-free): `EMBER_PROFILE_SCOPE("name")` — an RAII scope timer that records `{name, depth, start, end}` into a per-frame, thread-aware buffer; a `Profiler::beginFrame()/endFrame()` swaps buffers. Compiles to **nothing** unless `EMBER_ENABLE_PROFILING` is defined (off by default, like `EMBER_ENABLE_AUDIO`).
- **PROF-02** Instrument the key per-frame systems (script update, physics step, render submit, etc.) with `EMBER_PROFILE_SCOPE` so a populated scene yields a non-trivial tree.
- **PROF-03** `ProfilerPanel` flame graph: a hand-rolled ImGui draw-list renderer (no external library) showing the last frame's scope tree (nested bars, width ∝ duration, labeled, hover tooltip with ms).
- **PROF-04** Stats bar: frame time (ms), FPS, and `Renderer2D::stats()` (draw calls, quad count, vertices).
- **PROF-05** Rolling 120-frame frame-time history graph (`ImGui::PlotLines` or custom).
- **PROF-06** "Record" toggle captures N frames of full scope data into memory for inspection (and, optionally, dump to a file); off by default so profiling is zero-cost when not recording.

---

## 7. Tilemap editor (`editor/panels/TilemapEditorPanel`)

- **TME-01** Engine support: add `Tilemap::setTile(x,y,id)` and `resize(w,h)` (bounds-checked) so painting can mutate the grid; `at`/`isSolid`/`uvForTile` already exist.
- **TME-02** The `TilemapEditorPanel` appears when the (single) selected entity has a `Tilemap`; it hosts a **palette** showing the tileset texture sliced into its `columns×rows` grid — click a cell to set the active tile id.
- **TME-03** Paint mode: while active, left-click-drag in the Viewport maps the cursor world-position to a tile cell (via the tilemap's `Transform` + `tileWorldSize`) and writes the active tile.
- **TME-04** Erase mode (`E` / right-drag) writes id 0; Fill mode (bucket) flood-fills the contiguous same-id region under the cursor with the active tile.
- **TME-05** Undo is **per stroke**: a click-drag accumulates `{cell → oldId}` and pushes one `TilemapPaintCommand` on release (undo/redo restore the whole stroke). A 10-stroke sequence undoes cleanly without corruption.
- **TME-06** Tilemap (de)serialization: because `Tilemap` holds a `vector<u32>`, add custom serialize/deserialize so a painted tilemap round-trips inside the `.escene` (an inline data block — width/height/tileWorldSize/tileset + run-length or CSV tiles); register it so Save/Load preserves edits. (A referenced `.etilemap` form is optional.)
- **TME-07** Tilemap painting is a viewport input **mode**, mutually exclusive with camera navigation; exiting the mode restores normal viewport behavior.

---

## 8. Asset import pipeline (`engine/assets/importers/`)

- **IMP-01** `AssetImporter` interface + `AssetImportPipeline`: given a source file and its `.meta` (`AssetMeta{ importer, settings }`), produce/refresh the engine asset using the existing loaders; the pipeline is the single entry point the editor and `AssetManager` use to (re)build assets.
- **IMP-02** `TextureImporter`: settings for filter mode, wrap mode, generate-mips, max size, format (RGBA8/RGB8/R8); the Inspector shows these when a texture's `.meta` is selected and persists changes to the sidecar.
- **IMP-03** `FontImporter`: rasterize at a configured size set (default {12,14,16,24,32,48}) and cache an atlas per size.
- **IMP-04** "Re-import" (Inspector button / Asset Browser context) forces the pipeline to re-run for the selected asset and reloads live handles.
- **IMP-05** Dependency tracking: re-importing a tileset texture marks dependent tilemap assets dirty so they refresh.

---

## 9. Editor settings (`editor/panels/EditorSettingsPanel`)

- **SET-01** `Edit ▸ Preferences` opens a settings panel: theme (Dark / Light / Classic ImGui styles), grid snap size, rotate snap angle, camera pan/zoom speed, auto-save interval, default script-template path.
- **SET-02** Settings persist to `editor/preferences.yaml` (loaded at startup, saved on change/close); missing/!valid file falls back to defaults.
- **SET-03** Settings are consumed where relevant: theme → `ImGuiLayer`; snap → gizmo (GIZ-07); camera speed → `ViewportPanel`; auto-save → a timer that saves the current scene if it has a path and is dirty.

---

## 10. Debug overlays (View menu, `DebugDraw`/`Renderer2D`)

- **DBG-01** View-menu toggles (editor-only, **not** saved in the scene): Show Grid (configurable cell size), Show Entity Names (world-space labels via `Renderer2D::drawText`), Show Colliders (wire shapes from `BoxCollider2D`/`CircleCollider2D`), Show Camera Frustums (from `Camera2D`).
- **DBG-02** Overlays render in the viewport pass through `DebugDraw` (lines) / `Renderer2D` (text) and are skipped in Play mode (or toggleable there).
- **DBG-03** Overlay state lives in the editor (preferences or session), never in the serialized scene.

---

## 11. Test requirements (`tests/editor/`, headless via `ember_editor_core`)

ImGui/GL stays out of the test TUs; tests target the ImGui-free logic.

- **TST-01** `Selection` helper: add/remove/toggle/clear/contains/primary semantics, including primary reassignment when the primary is removed.
- **TST-02** `TransformGizmoCommand`: applying a delta to N entities then undo restores every entity's `Transform` exactly (one undo step).
- **TST-03** `BatchSetFieldCommand`: editing a shared field across N entities applies to all; undo reverts all.
- **TST-04** `TilemapPaintCommand`: a stroke of K cells applies, undo restores the K prior ids; a 10-stroke sequence undoes to the original grid. `Tilemap::setTile`/`resize` bounds-checked.
- **TST-05** Tilemap (de)serialization round-trips a painted grid (width/height/tiles/tileset) (TME-06).
- **TST-06** Prefab override diff: instantiate a prefab, change a field, compute overrides → exactly that field flagged; "revert" restores the source value; override set survives a scene save/load round-trip.
- **TST-07** Profiler buffer: nested `EMBER_PROFILE_SCOPE`s record correct parent/child depth + ordering and non-negative durations (compiled with `EMBER_ENABLE_PROFILING`); with profiling off, the macro expands to nothing.
- **TST-08** Gizmo snap math: snapping a translate delta to grid size G and a rotation to 15° yields the expected quantized values.

---

## 12. Non-Functional Requirements

- **NFR-01** ImGuizmo vendored under `third_party/imguizmo/` (or fetched), compiled into the `imgui`/editor target only; **no `ember_*` engine lib** sees ImGui or ImGuizmo. It is 3D-native so it carries into Epic 09 unchanged.
- **NFR-02** All new ImGui-free logic (`Selection`, gizmo math, the three new commands, prefab-override diff, tilemap edits) lives in `ember_editor_core` and is covered by `tests/editor/*`.
- **NFR-03** The profiler and import pipeline are **engine-side but ImGui/GL-free**; profiling is zero-cost when `EMBER_ENABLE_PROFILING` is off (default).
- **NFR-04** Builds clean `-Wall -Wextra -Wpedantic -Werror` on MinGW (Win) + clang (macOS); ImGui/ImGuizmo third-party TUs warnings-allowed. Editor builds with `EMBER_ENABLE_AUDIO` and `EMBER_ENABLE_PROFILING` both on and off.
- **NFR-05** Backwards compatible: v1 `.escene` files still load (new data — prefab overrides, tilemap blocks — is additive/optional); single-select behavior is unchanged when only one entity is selected.
- **NFR-06** Editor remains interactive (60 fps) on a few-hundred-entity scene with gizmos + overlays active.

---

## 13. Out of scope / open items

- **Nested prefabs** (prefab instances inside prefabs) — deferred (PFB-06).
- **3D gizmo usage** — ImGuizmo is 3D-ready, but only 2D (XY translate, Z rotate, XY scale) is exercised until Epic 09.
- **Collider/frustum debug shapes driven by live physics** — drawn from components now; full physics-debug integration is a later epic (DBG-01 notes this).
- **Multi-threaded / sampling profiler, chrome-trace export** — v2 ships a simple per-frame instrumented profiler; richer tooling later.
- **Animation/timeline editor, particle-curve editor, audio mixer UI** — not in this epic.
- **`.etilemap` external files** — v2 stores tilemaps inline in the scene; external referencing is optional.
- **Importer for audio/model formats** — only texture + font importers this epic.
- **Undo across scene load / play boundary** — history still clears on load/stop (v1 behavior).

---

## 14. Acceptance Criteria Summary

- [ ] Translate gizmo moves the entity in world space; a full drag is a single undo.
- [ ] Rotate gizmo snaps to 15° with `Ctrl` held; translate snaps to the grid size.
- [ ] Save entity as Prefab → delete it → drag the `.eprefab` from the Asset Browser → entity recreated correctly.
- [ ] Modify a prefab-instance field → shown bold → "Revert to Prefab" restores the source value; overrides survive save/reload.
- [ ] Profiler panel shows per-system timings (>0 ms) on a multi-system scene + a rolling frame-time graph.
- [ ] Tilemap paint draws tiles; a 10-stroke paint/erase/fill sequence undoes/redoes without corruption; the grid round-trips through save/load.
- [ ] Multi-select 5 entities → move with the gizmo → all 5 move → one undo reverts all 5; batch-edit a shared field applies to all.
- [ ] Texture `.meta` settings editable in the Inspector; "Re-import" re-runs the pipeline and refreshes the live asset.
- [ ] `ember_editor` builds clean (`-Werror`) on Windows + macOS; `tests/editor/*` pass; no engine library depends on ImGui/ImGuizmo.
