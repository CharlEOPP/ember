# Epic 07 — Editor v1

**Goal:** Deliver a functional in-process editor built on Dear ImGui. At the end of this epic a developer can open a scene, inspect and modify entities/components, play the scene, stop it, and save changes — all without leaving the tool.

**Depends on:** 03-ecs-and-scene, 04-renderer-2d, 05-input-and-assets, 06-scripting  
**Blocks:** 08-editor-v2

---

## Scope

### Application Shell
- `EditorApplication` — main entry point; owns `Window`, `AssetManager`, `SceneManager`, `EventBus`, `ImGuiLayer`
- `ImGuiLayer` — init/shutdown ImGui with GLFW + OpenGL backend; dockspace fullscreen layout; font loading (JetBrains Mono 14px default)
- Main loop:
  1. Poll events
  2. Update (skipped in Edit mode except editor systems)
  3. Render scene to framebuffer
  4. Render ImGui panels
  5. Swap

### Play / Edit State Machine
```
EDIT ──Play──► PLAYING ──Pause──► PAUSED
 ▲                │                  │
 └──────Stop──────┘◄────Resume───────┘
```
- **Edit mode:** ECS Update loop paused; editor camera active; scene mutations allowed
- **Play mode:** Scene deep-copied into a "play scene"; live ECS Update loop runs; editor overlays still render
- **Paused mode:** Loop paused mid-play; inspector shows live play-scene state (read-only)
- **Stop:** Play scene discarded; original scene state restored from the pre-play copy
- Toolbar: Play ▶ / Pause ⏸ / Stop ⏹ buttons; current mode shown in title bar

### Viewport Panel
- Scene rendered to `OpenGLFramebuffer` each frame
- Displayed as `ImGui::Image` filling the panel
- Mouse events forwarded to `EditorCamera` (pan with middle-mouse, zoom with scroll) in Edit mode
- Mouse events forwarded to game `InputManager` in Play mode
- Overlay: RendererStats (draw calls, quad count) in top-right corner in Edit mode
- Entity picking: click in viewport reads `entityID` from framebuffer's entity ID attachment → sets selected entity

### Scene Hierarchy Panel
- Displays entity tree with `ImGui::TreeNode`
- Expand/collapse per entity with children
- Selected entity highlighted
- Right-click context menu:
  - Create Empty Entity
  - Create Sprite Entity (adds Transform + SpriteRenderer)
  - Create Camera Entity
  - Duplicate
  - Delete
- Drag entities onto other entities to reparent (drop onto root area to un-parent)
- Double-click entity name to rename inline

### Inspector Panel
- Displays all components on the selected entity
- Each component rendered via its registered `drawUI(component&)` fn (or auto-generated field UI if none registered)
- Component header: name, collapse toggle, right-click → Remove Component
- "Add Component" button at bottom → searchable popup of all registered component types
- Built-in component UIs:
  - **Transform:** vec3 position, f32 rotation (degrees), vec3 scale; reset buttons per field
  - **SpriteRenderer:** texture asset picker (drag from AssetBrowser), color picker, layer int, flip checkboxes
  - **Camera2D:** size slider, near/far, isPrimary checkbox
  - **ScriptComponent subclasses:** reflected fields rendered as appropriate widgets (float → drag, bool → checkbox, string → input text, AssetHandle → asset picker)
- All Inspector mutations go through the `CommandHistory` (undo/redo)

### Asset Browser Panel
- Left pane: directory tree of `assets/`
- Right pane: icon grid of files in selected directory
- Texture files: thumbnail preview
- Double-click: open in system viewer (textures), or load scene
- Drag file onto Inspector field: auto-assigns if types match
- Right-click: Rename, Delete, Show in Explorer

### Command History (Undo/Redo)
- `Command` interface: `execute()`, `undo()`
- `CommandHistory`: linear stack, max 100 entries
- Built-in commands:
  - `SetComponentFieldCommand<T, F>` — generic field mutation
  - `CreateEntityCommand`
  - `DeleteEntityCommand`
  - `ReparentEntityCommand`
  - `RenameEntityCommand`
- Keyboard shortcuts: `Ctrl+Z` undo, `Ctrl+Y` / `Ctrl+Shift+Z` redo

### Scene Save / Load from Editor
- `File > Open Scene` — file dialog → `SceneManager::load`
- `File > Save Scene` — `YAMLSerializer::serialize` → write `.escene`
- `File > Save Scene As` — file dialog
- Unsaved-changes indicator in title bar (`*` suffix)
- On Play: auto-save to temp; on Stop: restore from temp (no user-visible save prompt)

### Keyboard Shortcuts
| Shortcut         | Action                |
|------------------|-----------------------|
| Ctrl+Z           | Undo                  |
| Ctrl+Y           | Redo                  |
| Ctrl+S           | Save Scene            |
| Ctrl+Shift+S     | Save Scene As         |
| Ctrl+D           | Duplicate selected    |
| Delete           | Delete selected       |
| F                | Focus viewport on selected entity |
| Ctrl+P           | Toggle Play           |

---

## Exit Criteria

- [ ] Open `sandbox/scenes/test.escene`, see entities in Hierarchy panel
- [ ] Click entity → Inspector shows all components with correct field values
- [ ] Modify a float field in Inspector → value changes in viewport in real-time
- [ ] Undo (Ctrl+Z) reverts the change; Redo (Ctrl+Y) re-applies it
- [ ] Create entity via right-click → appears in hierarchy → save → reload → entity persists
- [ ] Play → player controller responds to WASD → Stop → scene reverts to pre-play state
- [ ] Entity picking: click sprite in viewport → correct entity selected in Hierarchy
- [ ] Drag texture from Asset Browser onto SpriteRenderer texture field → sprite updates

---

## Key Files Created

```
editor/main.cpp
editor/EditorApplication.h/.cpp
editor/ImGuiLayer.h/.cpp
editor/PlayModeController.h/.cpp
editor/CommandHistory.h/.cpp
editor/panels/ViewportPanel.h/.cpp
editor/panels/SceneHierarchyPanel.h/.cpp
editor/panels/InspectorPanel.h/.cpp
editor/panels/AssetBrowserPanel.h/.cpp
editor/panels/MenuBar.h/.cpp
editor/commands/SetFieldCommand.h
editor/commands/CreateEntityCommand.h
editor/commands/DeleteEntityCommand.h
editor/commands/ReparentCommand.h
assets/fonts/JetBrainsMono-Regular.ttf
```

---

## Notes & Decisions

- Entity picking uses a second framebuffer attachment rendered in the same pass as the main scene — the `entityID` float (cast to int) is stored per fragment. This reuses Epic 04's vertex layout `entityID` field.
- The editor and game run in the same process with no IPC — this is intentional for simplicity. Isolation (editor ↔ game in separate processes) is a stretch goal for a much later milestone.
- ImGui docking layout is saved to `editor/layout.ini` via ImGui's built-in mechanism
- File dialogs use `nativefiledialog-extended` (NFD) — vendored under `third_party/`
