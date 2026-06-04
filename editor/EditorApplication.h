#pragma once
#include "ember/events/EventBus.h"
#include "ember/assets/AssetManager.h"
#include "ImGuiLayer.h"
#include "EditorContext.h"
#include "CommandHistory.h"
#include "FileBrowser.h"
#include "PlayModeController.h"
#include "panels/ViewportPanel.h"
#include "panels/SceneHierarchyPanel.h"
#include "panels/InspectorPanel.h"
#include "ember/input/InputManager.h"

#include <memory>
#include <string>

namespace ember {

class Window;
class Scene;
class SceneManager;

// Top-level editor: owns the window, RHI/Renderer2D, asset + scene management,
// the ImGui layer, and the shared EditorContext. Runs the main loop and draws
// the docked panels.
class EditorApplication {
public:
    EditorApplication();
    ~EditorApplication();

    EditorApplication(const EditorApplication&)            = delete;
    EditorApplication& operator=(const EditorApplication&) = delete;

    void run();

private:
    void beginDockspace();   // fullscreen host window + DockSpace + menu bar + toolbar
    void endDockspace();
    void drawPanels(f32 dt);
    void handleShortcuts();
    void togglePlay();

    // Scene file operations.
    void newScene();
    void openSceneFile(const std::string& path);
    void saveSceneToPath(const std::string& path);
    void saveOrPrompt();     // Ctrl+S: save to current path, or prompt if none
    void updateTitle();

    bool        m_shouldClose = false;
    std::string m_scenePath;             // current file ("" = untitled)
    std::string m_lastTitle;

    std::unique_ptr<Window>       m_window;
    EventBus                      m_bus;
    AssetManager                  m_assets;
    std::unique_ptr<SceneManager> m_scenes;
    std::unique_ptr<Scene>        m_scene;   // active edit scene (SceneManager-backed in Phase 5)
    ImGuiLayer                    m_imgui;
    CommandHistory                m_history;
    EditorContext                 m_ctx;
    PlayModeController            m_play;
    InputManager                 m_input;
    ViewportPanel                m_viewport;
    SceneHierarchyPanel          m_hierarchy;
    InspectorPanel               m_inspector;
    FileBrowser                  m_fileBrowser;
};

} // namespace ember
