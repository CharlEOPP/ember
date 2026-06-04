# Epic 07 — Editor v1: Requirements

**Status:** Draft  
**Epic:** 07-editor-v1  
**Depends on:** 03-ecs-and-scene, 04-renderer-2d, 05-input-and-assets, 06-scripting, 12-2d-gameplay-and-audio  
**Blocks:** 08-editor-v2

---

## 1. Overview

Deliver a functional, in-process **scene editor** built on Dear ImGui (docking branch). At the end of this epic a developer can open a scene, see its entity tree, select an entity and edit its components live, play/pause/stop the scene, undo/redo edits, and save back to disk — all from one window. The editor and game run in the **same process** (no IPC); process isolation is a far-later stretch goal.

The editor is a new top-level `editor/` application target. It reuses the engine's existing public surfaces rather than introducing parallel systems: `Window`, `RHI`/`Renderer2D`, `World`/`Scene`/`SceneManager`, the `ComponentRegistry` reflection/serialization tables, `AssetManager`, `Input`, and `EventBus`. The only new third-party code is **Dear ImGui** (already declared in `cmake/Dependencies.cmake`) and an optional native file-dialog library.

This is **v1**: a single docked layout (Viewport, Hierarchy, Inspector, Asset Browser, menu/toolbar), reflection-driven inspector widgets with a few hand-written component UIs, and a linear undo/redo history. Gizmos, multi-select, prefab editing, and a polished asset pipeline are Epic 08.

### 1.1 Grounding in the current codebase

| Existing facility (epic) | How the editor uses it |
|--------------------------|------------------------|
| `Window` (GLFW) with `getNativeHandle() → GLFWwindow*`, `setEventCallback`, `getWidth/Height` (02) | passes the native handle to the ImGui GLFW backend; forwards resize to the viewport framebuffer |
| `RHI` + `IFramebuffer{ bind/unbind/resize/getColorAttachment() }`, `FramebufferSpec{ width,height,depth }`, `createFramebuffer` (04) | renders the scene off-screen; `getColorAttachment()` is shown as `ImGui::Image` in the Viewport panel |
| `Renderer2D` vertex carries a per-vertex `entityID` float; `drawQuad(..., entityID)` already plumbed (04) | basis for click-to-pick — **but** the framebuffer has no integer/ID attachment or pixel readback yet (see VP-04, a required RHI extension) |
| `World` (entt): `create/destroy/emplace<T>/get<T>/tryGet<T>/view`, `Tag`, `Transform`, `Parent`/`Children`, `WorldTransform` + `TransformSystem`, `Scene::create/getWorldTransform` (03) | hierarchy tree, selection, reparenting, entity creation/deletion |
| `ComponentRegistry` + `ComponentMeta{ name, type, serialize, deserialize, drawUI }` with `byName/byType/all()` (03) | `drawUI` is the **reserved, currently-null** inspector hook this epic fills; `all()` powers the "Add Component" list |
| `EMBER_REFLECT(T){ EMBER_FIELD(x); }` free-function reflection + the YAML read/write visitors (03/05/06) | a new **ImGui inspect visitor** renders each reflected field as a widget (mirrors the YAML visitors' field-type coverage) |
| `SceneManager(EventBus&)` with injected `setLoader(Loader)`, `load/loadAdditive/getActive/transition/endFrame` (03) | the editor wires `setLoader` to `YAMLDeserializer::deserialize`; Open/Save go through it |
| `YAMLSerializer::serialize(const Scene&) → string`, `YAMLDeserializer::deserialize(string, Scene&)`, `serializePrefab` (05) | Save / Save As write the string to a `.escene` file; Open parses it |
| `ComponentSerialization`: `registerComponentType<T>(name)`, `installAssetSerializationResolver(AssetManager&)` (05/06) | scene round-trips through the same registered component set the runtime uses |
| `AssetManager`: `load<T>`, `get<T>`, `pathOf`, `loadByType`, `FileWatcher` hot reload (05) | Asset Browser lists `assets/`; drag-to-assign resolves handles; thumbnails for textures |
| `Input` mouse facade: `getMousePosition`, `isMouseButtonDown/Pressed/Released` (05) | viewport camera control in Edit mode; ImGui owns input when a panel is focused |
| `EditorCamera` (`Camera.h`) with `setViewportSize`, `zoom`, pan, `viewProjection()` (04) | the Edit-mode viewport camera |
| All Epic 12 components already registered via `registerRenderComponents()`/`registerPhysics2DComponents()` (12) | appear automatically in the Inspector and Add-Component list with reflection-driven UIs |

### 1.2 Key design decisions

- **Inspector is reflection-driven, with optional hand-written overrides.** `ComponentMeta.drawUI` is `std::function<void(World&, Entity)>` and lives in `ember_ecs` (no ImGui leak). The editor maintains its **own** type→draw table (it cannot mutate the const registry, and ImGui must not enter the engine libs). For every type in `ComponentRegistry::all()`, the default entry runs an **`ImGuiInspectVisitor`** over the component's reflected fields; specific types (Transform, SpriteRenderer, Camera2D, …) get registered overrides via an editor-side `EMBER_REGISTER_INSPECTOR(T, fn)`. A component with no reflection simply shows "no editable fields".
- **Entity picking needs a small RHI extension.** The current `IFramebuffer` exposes only a single color attachment and no readback. v1 adds an **integer ID attachment** to the framebuffer (driven by the existing `entityID` vertex attribute) plus `IFramebuffer::readPixelInt(attachment, x, y)`. A **CPU-side fallback** (ray vs `WorldTransform`-projected sprite/collider AABBs) is the contingency if the GL ID attachment slips; the picking *interface* the panels call is identical either way (VP-04).
- **All structural/field mutations go through `CommandHistory`.** Inspector edits, create/delete/duplicate/reparent/rename are `Command` objects so undo/redo is uniform. Live ImGui drags coalesce into one command per interaction (begin-edit → end-edit), not one per frame.
- **Play mode snapshots the scene.** Entering Play serializes the active scene to an in-memory `.escene` string (reusing `YAMLSerializer`); Stop restores by deserializing it back. This reuses the existing serialization path instead of a bespoke deep-copy and guarantees "what you save is what you play".
- **No new engine module.** The editor is an application (`editor/`), not an `ember_*` library; engine libs gain **no** dependency on ImGui. The only engine-side change is the RHI framebuffer ID attachment (VP-04), which is generally useful and stays GL-internal.

---

## 2. Definitions

- **Edit mode / Play mode / Paused** — the three editor run states (see §4).
- **Active scene** — `SceneManager::getActive()`; the scene the panels operate on.
- **Selection** — the currently selected `Entity` (single-select in v1; `NULL_ENTITY` = nothing selected).
- **Panel** — one docked ImGui window (Viewport, Hierarchy, Inspector, Asset Browser).
- **Inspector visitor** — the reflection visitor that emits an ImGui widget per `EMBER_FIELD`.
- **Command** — a reversible editor action with `execute()`/`undo()`, stored in the `CommandHistory`.
- **Dirty** — the active scene has unsaved changes (title-bar `*`).

---

## 3. Application shell & main loop (`editor/`)

- **SHELL-01** `EditorApplication` is the entry point; it owns the `Window`, `RHI` init, `Renderer2D`, `AssetManager`, `SceneManager` (+ `EventBus`), `ImGuiLayer`, the selection state, and the `CommandHistory`.
- **SHELL-02** `ImGuiLayer` initializes ImGui with the GLFW (`Window::getNativeHandle()`) + OpenGL3 backends, enables docking, builds a fullscreen dockspace, and loads a default font (JetBrains Mono 14px; falls back to the ImGui default font if the TTF is absent). Shutdown tears both backends down cleanly.
- **SHELL-03** Per-frame loop order: (1) poll events; (2) `ImGuiLayer::begin()` new-frame; (3) **update** — in Play mode run the full `SystemScheduler`/`ScriptSystem`, in Edit mode run only editor systems (camera, `TransformSystem`); (4) render the scene into the viewport framebuffer; (5) draw the dockspace + all panels; (6) `ImGuiLayer::end()` render-draw-data; (7) swap.
- **SHELL-04** The dockspace layout persists across runs via ImGui's built-in `imgui.ini` (stored as `editor/layout.ini`); a "Reset Layout" menu item restores the default arrangement.
- **SHELL-05** The editor degrades gracefully when a scene is not loaded (empty hierarchy/inspector, viewport shows clear color) — no crash on first launch.

---

## 4. Play / Edit / Pause state machine (`editor/PlayModeController`)

```
EDIT ──Play──► PLAYING ──Pause──► PAUSED
 ▲               │                  │
 └─────Stop──────┴────Resume────────┘
```

- **PLAY-01** **Edit mode** (default): the ECS update loop is paused (no `ScriptSystem`/physics); the `EditorCamera` is active; scene mutations and the `CommandHistory` are enabled.
- **PLAY-02** **Play**: snapshot the active scene to an in-memory `.escene` string, then run the live update loop (scripts, physics, animation, particles, audio). Editor overlays still render. Picking/selection remain available; inspector edits during play affect the **play** scene (not persisted).
- **PLAY-03** **Pause**: freeze the update loop mid-play; the inspector shows live play-scene state (read-only badge); Resume continues.
- **PLAY-04** **Stop**: discard the play scene and restore the pre-play snapshot (deserialize it back into the active scene); selection is preserved by stored entity id where possible; the `CommandHistory` from Edit mode is intact (play-mode changes are **not** added to it).
- **PLAY-05** A toolbar shows Play ▶ / Pause ⏸ / Stop ⏹ with the current mode reflected in the title bar; `Ctrl+P` toggles Play/Stop.
- **PLAY-06** Entering Play with unsaved edits does **not** force a save (the snapshot covers restore); the dirty flag is preserved across the play/stop cycle.

---

## 5. Viewport panel (`editor/panels/ViewportPanel`)

- **VP-01** The active scene is rendered into an `IFramebuffer` sized to the panel's content region; the framebuffer resizes when the panel resizes (and on window resize). The color attachment is displayed via `ImGui::Image`, filling the panel with correct aspect ratio.
- **VP-02** In **Edit mode**, mouse over the focused viewport drives the `EditorCamera`: middle-mouse (or alt+left) pans, scroll zooms; the camera's viewport size tracks the panel. ImGui's `WantCaptureMouse` is respected so dragging panels never moves the camera.
- **VP-03** In **Play mode**, viewport input is forwarded to the game `InputManager` instead of the editor camera, so gameplay controls work inside the panel.
- **VP-04** **Entity picking:** clicking in the viewport selects the entity under the cursor. Primary mechanism: a second **integer ID attachment** on the viewport framebuffer (written from the existing `Renderer2D` `entityID` vertex attribute) read back via a new `IFramebuffer::readPixelInt(attachment, x, y)`; this requires extending `FramebufferSpec` (add an optional `idAttachment` flag) and the GL framebuffer impl. **Fallback** (if the GL ID path slips): CPU ray-vs-AABB against each entity's `WorldTransform`-projected sprite/collider bounds. Either way the panel calls one `pickEntity(viewportX, viewportY) → Entity` API.
- **VP-05** Overlay (Edit mode): `RendererStats` (draw calls, quad count) in a corner; an unobtrusive mode/FPS readout. Overlays are skipped or dimmed in Play mode.
- **VP-06** Picking and camera control account for the panel's screen offset and the framebuffer/panel scale so clicks map to the correct world position at any panel size or window DPI.

---

## 6. Scene Hierarchy panel (`editor/panels/SceneHierarchyPanel`)

- **HIER-01** Displays the active scene's entities as a tree using `Parent`/`Children`; roots at top level, children nested under `ImGui::TreeNode`, expand/collapse per entity. Entity label is its `Tag.name` (falls back to `Entity <id>`).
- **HIER-02** Clicking an entity selects it (updates shared selection); the selected row is highlighted; clicking empty space clears selection.
- **HIER-03** Right-click context menu: **Create Empty**, **Create Sprite** (Transform + SpriteRenderer), **Create Camera** (Camera2D), **Duplicate**, **Delete**. Create/Duplicate/Delete go through commands (CMD-03..05).
- **HIER-04** Drag an entity onto another to **reparent** (drop onto empty/root area to un-parent); reparenting is a command and preserves world transform where feasible (adjust local transform so the entity doesn't visually jump, or document that v1 keeps local transform — see open items).
- **HIER-05** Double-click a name to rename inline (commits on Enter/focus-loss; a command).
- **HIER-06** Deleting a parent deletes its subtree (matching `Scene`/`World` destroy semantics); selection clears if the selected entity was removed.

---

## 7. Inspector panel (`editor/panels/InspectorPanel`)

- **INS-01** With an entity selected, lists every component on it (iterate `ComponentRegistry::all()` / `byType`, testing presence per entity). Each component is a collapsible header showing its registered name.
- **INS-02** Each component body is drawn by the editor's type→draw table: a **registered override** if present, else the default **`ImGuiInspectVisitor`** which walks the component's reflected fields and emits a widget per type — `f32`→drag float, `i32/u32/u64`→drag int, `bool`→checkbox, `std::string`→input text, `glm::vec2/3/4`→multi-component drag (vec4 also offers a color picker), `Entity`→entity drop target, `AssetHandle<T>`→asset picker (drag from Asset Browser / path field), enum→combo. Unsupported field types are shown read-only/skipped (never a crash), mirroring the YAML visitors' graceful degradation.
- **INS-03** Built-in hand-written overrides for the most-used components: **Transform** (position vec3, rotation in degrees, scale vec3, per-field reset), **SpriteRenderer** (texture asset picker, color picker, layer, flipX/Y, uvRect), **Camera2D** (size, near/far, isPrimary). Script components and Epic 12 components (RigidBody2D, colliders, ParticleEmitter, UI*) use the reflection default until/unless an override is added.
- **INS-04** Component header right-click → **Remove Component** (a command). A bottom **"Add Component"** button opens a searchable popup listing all registered component types not already present; adding is a command.
- **INS-05** **Every** inspector mutation is routed through the `CommandHistory`. Continuous ImGui edits (drags/sliders) coalesce: a single command spanning `IsItemActivated()` → `IsItemDeactivatedAfterEdit()`, capturing before/after values, so one drag = one undo step.
- **INS-06** The inspector is read-only (visibly badged) while **Paused**; during **Play** it edits the live play scene without touching the Edit-mode history.

---

## 8. Asset Browser panel (`editor/panels/AssetBrowserPanel`)

- **AB-01** Left pane: a directory tree rooted at the project `assets/` folder. Right pane: an icon grid of the selected directory's files; navigation via double-click into folders + a breadcrumb/up control.
- **AB-02** Texture files show a thumbnail (loaded through `AssetManager`); other types show a type icon. Names are truncated with tooltips.
- **AB-03** Files are **drag sources**: dropping a texture onto a SpriteRenderer texture field (or any matching `AssetHandle<T>` field) assigns it (type-checked; mismatches are rejected). Dropping a `.escene` onto the viewport/hierarchy (or double-clicking it) opens that scene.
- **AB-04** Right-click file menu: **Rename**, **Delete**, **Show in Explorer/Finder** (best-effort per OS). Destructive ops confirm.
- **AB-05** The browser reflects external changes (new/removed files) on refocus or via the existing `FileWatcher`; no manual refresh required for the common case.

---

## 9. Command history — undo/redo (`editor/CommandHistory`)

- **CMD-01** `Command` interface: `execute()`, `undo()`, and a short `name()` for UI/debug. `CommandHistory` is a linear undo/redo stack (cap ~100 entries; oldest dropped past the cap); pushing a new command after undo truncates the redo tail.
- **CMD-02** `SetFieldCommand` — generic component-field mutation capturing before/after values (used by INS-05). Coalesces a drag interaction into one entry.
- **CMD-03** `CreateEntityCommand` / **CMD-04** `DeleteEntityCommand` (stores enough to restore the entity + its components + parentage on undo) / **CMD-05** `ReparentEntityCommand` / **CMD-06** `RenameEntityCommand` / `DuplicateEntityCommand`.
- **CMD-07** Shortcuts: `Ctrl+Z` undo, `Ctrl+Y` / `Ctrl+Shift+Z` redo. Undo/redo update selection and mark the scene dirty appropriately.
- **CMD-08** The `CommandHistory` and the command-construction logic are **engine-independent of ImGui** so they can be unit-tested headless (TST-01).

---

## 10. Scene save/load, menu bar & shortcuts (`editor/panels/MenuBar`)

- **FILE-01** `File ▸ Open Scene…` loads a `.escene` via `SceneManager::load` (loader wired to `YAMLDeserializer::deserialize`); replaces the active scene (prompting if dirty).
- **FILE-02** `File ▸ Save Scene` writes `YAMLSerializer::serialize(activeScene)` to the current path; `Save Scene As…` chooses a new path. `New Scene` creates an empty active scene.
- **FILE-03** Unsaved changes show a `*` in the title bar; saving clears it. Closing/opening with unsaved changes prompts to save/discard/cancel.
- **FILE-04** File chooser: an **in-ImGui file browser** is the default (no new native dependency); **nativefiledialog-extended (NFD)** is an optional vendored upgrade (open item §13). Either way Open/Save call the same path-returning helper.
- **KEY-01** Global shortcuts (respecting ImGui text-input focus): `Ctrl+S` Save, `Ctrl+Shift+S` Save As, `Ctrl+Z`/`Ctrl+Y` undo/redo, `Ctrl+D` Duplicate, `Delete` Delete selected, `F` focus viewport camera on selection, `Ctrl+P` toggle Play.

---

## 11. Test requirements (`tests/editor/`)

The editor is GL+GLFW+ImGui and **cannot run headless** in CI; tests target the engine-independent logic and the new RHI surface's contract.

- **TST-01** `CommandHistory` (no ImGui): execute pushes + applies; undo reverts; redo re-applies; redo tail truncates on new command after undo; cap evicts oldest. `SetFieldCommand` round-trips a component field value.
- **TST-02** Create/Delete/Reparent/Rename/Duplicate commands undo to the exact prior `World` state (component set, parentage, tag) — asserted on a headless `World`/`Scene`.
- **TST-03** Play→Stop restore: serialize a scene, mutate it, restore from the snapshot string → entities/components/fields match the pre-play state (reuses the serialization round-trip).
- **TST-04** Inspector model: the reflection inspect visitor visits exactly the `EMBER_FIELD`s of a sample component with the correct type tags (assert via a recording visitor — no ImGui), and skips unsupported types without error.
- **TST-05** Picking math (CPU fallback path): ray/click maps to the correct entity given known `WorldTransform`s and overlapping bounds (front-most wins); and viewport→world coordinate mapping is correct at non-default panel sizes.
- **TST-06** "Add Component" lists exactly the registered components absent from the entity; adding then removing leaves the `World` unchanged.

---

## 12. Non-Functional Requirements

- **NFR-01** New application target(s) under `editor/` (`ember_editor`); it links the `ember` aggregate + ImGui (core + `imgui_impl_glfw` + `imgui_impl_opengl3`, compiled by the editor since ImGui is fetched-but-not-built). **No `ember_*` engine library gains an ImGui dependency.**
- **NFR-02** The only engine-side change is the RHI framebuffer **ID attachment + `readPixelInt`** (VP-04), kept GL-internal; existing single-attachment users are unaffected (the ID attachment is opt-in via `FramebufferSpec`).
- **NFR-03** Editor interactions are non-destructive by default: all model changes go through commands; Play/Stop never corrupts the on-disk scene.
- **NFR-04** Reasonable interactivity on a typical 2D scene (hundreds–low-thousands of entities) at 60 fps; the hierarchy/inspector avoid per-frame heap churn where practical (no hard perf gate this epic).
- **NFR-05** Builds clean under `-Wall -Wextra -Wpedantic -Werror` (ImGui third-party sources may be compiled warnings-suppressed, as miniaudio/GL deps already are). `tests/editor/*` pass under `debug`/`release`/`sanitize`.
- **NFR-06** ImGui is vendored via the existing FetchContent (docking branch) and is the **only** new mandatory dependency; the editor must build with `EMBER_ENABLE_AUDIO` either ON or OFF.

---

## 13. Out of scope / open items

- **Transform/translate/rotate/scale gizmos** in the viewport (ImGuizmo) — Epic 08.
- **Multi-select**, box-select, and multi-entity edits — v1 is single-select.
- **Prefab authoring/editing UI** — serialization supports prefabs (`serializePrefab`), but an editing workflow is Epic 08.
- **NFD vendoring** — default is an in-ImGui file browser; vendoring `nativefiledialog-extended` under `third_party/` is an optional polish item.
- **Reparent transform preservation** — open decision: keep local transform (entity may visually jump) vs recompute local from world on reparent (no jump). v1 may ship the simpler keep-local and note it; the no-jump version is preferred if cheap.
- **GPU entity-ID picking vs CPU fallback** — primary is the GL ID attachment (VP-04); if it slips, ship CPU ray/AABB picking with the same `pickEntity` interface and finish the GL path in Epic 08.
- **`drawUI` override registration coverage** — only the high-traffic components get hand-written UIs in v1; everything else uses the reflection default.
- **Editor preferences / theming / multiple layouts** — beyond the single saved dockspace — later.
- **Asset import pipeline** (reimport, metadata editing beyond `.meta`) — later.

---

## 14. Acceptance Criteria Summary

- [ ] Launch the editor, `File ▸ Open` a `.escene` → entities appear in the Hierarchy panel.
- [ ] Select an entity → Inspector shows all its components with correct field values.
- [ ] Edit a Transform/float field in the Inspector → the change is visible in the Viewport in real time.
- [ ] `Ctrl+Z` reverts the edit; `Ctrl+Y` re-applies it (one drag = one undo step).
- [ ] Right-click ▸ Create entity → it appears in the Hierarchy → Save → reload → it persists.
- [ ] Play → a `PlayerController`/physics body responds to input → Stop → scene reverts to the pre-play state.
- [ ] Click a sprite in the Viewport → the correct entity is selected in the Hierarchy.
- [ ] Drag a texture from the Asset Browser onto a SpriteRenderer texture field → the sprite updates.
- [ ] Reparent via drag, rename inline, duplicate, and delete all work and are undoable.
- [ ] `ember_editor` builds clean (`-Werror`, ImGui warnings-suppressed); `tests/editor/*` pass; no engine library depends on ImGui.
