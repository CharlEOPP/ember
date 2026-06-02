# Epic 08 — Editor v2

**Goal:** Polish and extend the editor to professional quality. Gizmos make transform editing intuitive. Prefabs enable reuse. The profiler exposes performance. Tilemaps are editable. The editor becomes a tool you'd actually want to ship a game with.

**Depends on:** 07-editor-v1  
**Blocks:** 09-3d-foundation (gizmo system must be 3D-ready)

---

## Scope

### Transform Gizmos
- Integrate **ImGuizmo** for translate, rotate, scale gizmos in the Viewport
- Toolbar toggle: T (translate), R (rotate), S (scale), with keyboard shortcuts matching standard DCC tools
- Local vs. world space toggle
- Multi-select: gizmo operates on the bounding center of all selected entities; each entity transforms relative to its own pivot
- Gizmo operations generate `TransformGizmoCommand` pushed to `CommandHistory` (single undo for a drag operation, not per-frame)
- Snap: hold Ctrl to snap translate to grid (configurable grid size, default 1 unit), rotate to 15° increments

### Profiler Panel
- `ProfilerPanel` displays per-frame timing captured by `EMBER_PROFILE_SCOPE` macros
- Flame graph view (ImGui draw list, custom renderer — no external profiler dependency required)
- Stats bar: Frame time (ms), FPS, Draw Calls, Quad Count, Vertices
- History graph: rolling 120-frame history of frame time
- "Record" toggle — captures N frames of full scope data for offline analysis

### Prefab System
- A **Prefab** is a `.eprefab` YAML file containing a single entity tree (same format as a scene entity subtree)
- `Scene::instantiate(prefabHandle, position)` → Entity (existing API, now fully wired)
- In the editor:
  - Right-click entity in Hierarchy → "Save as Prefab" → writes `.eprefab` to assets directory
  - Drag `.eprefab` from Asset Browser into Viewport or Hierarchy → instantiates at origin
  - Prefab instances highlighted in Hierarchy with a blue icon
- **Prefab overrides:** instance fields that differ from the prefab source are shown in bold in the Inspector; right-click → "Revert to Prefab" restores them
- No nested prefab support in this epic (deferred)

### Multi-Selection
- Hold Shift/Ctrl to add/remove entities from selection
- Selection stored as `std::unordered_set<Entity>` in `EditorContext`
- Hierarchy highlights all selected entities
- Inspector in multi-select mode: shows components shared by all selected entities; editing a shared field applies to all (generates one `BatchSetFieldCommand`)
- Delete, Duplicate, Reparent all operate on the full selection

### Tilemap Editor
- Dedicated `TilemapEditorPanel` when a `TilemapComponent` entity is selected
- Palette panel: displays tileset texture sliced into tile grid; click to select active tile
- Paint mode: left-click drag in Viewport paints selected tile onto hovered grid cell
- Erase mode: right-click or 'E' key
- Fill mode (bucket): flood-fill contiguous region with selected tile
- Undo/redo per tile-paint stroke (not per cell — a stroke is one click-drag)
- Export: tilemap data saved inside the `.escene` file as an inline data block or referenced `.etilemap` file

### Asset Import Pipeline
- `AssetImporter` — processes raw source files into engine-ready formats with settings from `.meta`
- Texture importer settings UI in Inspector when a texture `.meta` is selected:
  - Filter mode, wrap mode, generate mips, max size, format (RGBA8, RGB8, R8)
- Font importer settings: rasterize at sizes [12, 14, 16, 24, 32, 48] and cache atlas per size
- "Re-import" button forces re-run of importer
- Asset dependency tracking: if a tileset texture is re-imported, dependent `TilemapAsset` is marked dirty

### Editor Settings
- `EditorSettingsPanel` (Edit → Preferences):
  - Theme: Dark / Light / Classic (ImGui themes)
  - Grid snap size
  - Camera pan/zoom speed
  - Auto-save interval
  - Default script template path
- Settings persisted to `editor/preferences.yaml`

### Debug Overlays
- Overlay toggles in View menu:
  - Show Colliders (wire shapes, wired to physics in a future epic)
  - Show Camera Frustums
  - Show Entity Names (world-space labels)
  - Show Grid (configurable cell size)
- Debug overlay state is editor-only, not saved in scene

---

## Exit Criteria

- [ ] Translate gizmo moves entity correctly in world space; single undo reverts the full drag
- [ ] Rotate gizmo snaps to 15° with Ctrl held
- [ ] Save entity as Prefab → delete entity → drag `.eprefab` from Asset Browser → entity recreated correctly
- [ ] Modify prefab instance field in Inspector → shown in bold → "Revert to Prefab" restores original value
- [ ] Profiler panel shows per-system timings with >0ms on a 5-system scene
- [ ] Tilemap paint tool draws tiles correctly; 10-stroke undo sequence works without corruption
- [ ] Multi-select 5 entities → move with gizmo → all 5 move → undo reverts all 5

---

## Key Files Created

```
editor/panels/ProfilerPanel.h/.cpp
editor/panels/TilemapEditorPanel.h/.cpp
editor/panels/EditorSettingsPanel.h/.cpp
editor/gizmos/TransformGizmo.h/.cpp
editor/commands/TransformGizmoCommand.h
editor/commands/TilemapPaintCommand.h
editor/commands/BatchSetFieldCommand.h
engine/assets/importers/TextureImporter.h/.cpp
engine/assets/importers/FontImporter.h/.cpp
engine/assets/AssetImportPipeline.h/.cpp
editor/preferences.yaml
```

---

## Notes & Decisions

- ImGuizmo is vendored under `third_party/imguizmo/`; it is 3D-native so the gizmo will work correctly without changes when 3D is added in Epic 09
- The profiler flame graph is a custom ImGui DrawList implementation (300–400 lines); it avoids adding a heavy profiler library dependency
- Prefab overrides are stored per-instance as a map of `{componentType, fieldName} → overrideValue` serialized alongside the entity in the scene file
- The tilemap editor shares the Viewport — it does not open a separate window; brush painting is an editor input mode that is mutually exclusive with normal viewport navigation
