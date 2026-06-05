#include "EditorApplication.h"

#include "ember/core/Log.h"
#include "ember/core/Profiler.h"
#include "ember/platform/Window.h"
#include "ember/platform/Timer.h"
#include "ember/renderer/RHI.h"
#include "ember/renderer/Renderer2D.h"
#include "ember/renderer/DebugDraw.h"
#include "ember/scene/Scene.h"
#include "ember/scene/SceneManager.h"
#include "ember/serialization/YAMLSerializer.h"
#include "ember/serialization/AssetResolver.h"
#include "ember/renderer/RenderComponents.h"
#include "ember/physics2d/Physics2DComponents.h"
#include "ember/scene/PrefabInstance.h"
#include "ember/ecs/World.h"
#include "EditorInspectors.h"
#include "Commands.h"

#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>

namespace ember {

EditorApplication::EditorApplication()
    : m_play(m_assets), m_viewport(m_assets) {
    WindowSpec spec;
    spec.title  = "Ember Editor";
    spec.width  = 1600;
    spec.height = 900;
    spec.vsync  = true;
    m_window = std::make_unique<Window>(spec);

    RHI::init(RHIBackend::OpenGL);
    RHI::setClearColor(0.10f, 0.10f, 0.12f);
    RHI::setViewport(0, 0, m_window->getWidth(), m_window->getHeight());
    Renderer2D::init();
    DebugDraw::init();   // no-op in Release; powers the editor debug overlays

    m_scenes = std::make_unique<SceneManager>(m_bus);
    m_scene  = std::make_unique<Scene>("untitled");

    m_ctx.scene   = m_scene.get();
    m_ctx.assets  = &m_assets;
    m_ctx.history = &m_history;

    // Register components with the scene serializer + the asset-handle resolver
    // so Save/Load round-trips sprites, physics bodies, handles, etc.
    registerRenderComponents();
    registerPhysics2DComponents();
    installAssetSerializationResolver(m_assets);
    registerEditorInspectors();

    m_input.init(*m_window);   // becomes the active manager for the Input:: facade
    m_assetBrowser.setOpenSceneCallback([this](const std::string& path) { openSceneFile(path); });

    m_settings.load(kPrefsPath);   // defaults if missing

    m_imgui.init(*m_window);
    applySettings();               // theme + gizmo snap + camera speeds
    EMBER_LOG_INFO("Ember Editor initialised");
}

EditorApplication::~EditorApplication() {
    m_imgui.shutdown();
    DebugDraw::shutdown();
    Renderer2D::shutdown();
    EMBER_LOG_INFO("Ember Editor shutting down");
}

void EditorApplication::run() {
    Timer frameTimer;
    while (!m_window->shouldClose() && !m_shouldClose) {
        const auto dt = static_cast<f32>(frameTimer.elapsed());
        frameTimer.reset();

        Profiler::instance().beginFrame();

        m_window->pollEvents();
        m_input.update();   // roll input edges (game scripts read these while playing)

        // While playing, edits bypass the command history (they're discarded on
        // Stop's snapshot restore), keeping the edit-mode undo stack intact.
        const bool playing = m_play.isPlaying();
        m_ctx.history = playing ? nullptr : &m_history;
        m_ctx.playing = playing;
        if (playing) {
            EMBER_PROFILE_SCOPE("Play::update");
            m_play.update(*m_scene, dt);
        }

        // Default backbuffer clear (the dockspace host covers it; panels render
        // into their own framebuffers).
        RHI::setViewport(0, 0, m_window->getWidth(), m_window->getHeight());
        RHI::clear();

        m_imgui.begin();
        beginDockspace();
        drawPanels(dt);

        // File browser modal (Open / Save As / Save-as-Prefab).
        if (m_fileBrowser.draw()) {
            if (m_fileBrowser.mode() == FileBrowser::Mode::Open) {
                openSceneFile(m_fileBrowser.result());
            } else if (m_pendingPrefabEntity != NULL_ENTITY) {
                auto res = YAMLSerializer::savePrefabToFile(*m_scene, m_pendingPrefabEntity,
                                                            m_fileBrowser.result());
                if (res) {
                    World& w = m_scene->world();
                    PrefabInstance* pi = w.tryGet<PrefabInstance>(m_pendingPrefabEntity);
                    if (!pi) pi = &w.emplace<PrefabInstance>(m_pendingPrefabEntity);
                    pi->source = m_fileBrowser.result();
                    m_ctx.dirty = true;
                } else {
                    EMBER_LOG_ERROR("Save prefab failed: {}", res.error());
                }
                m_pendingPrefabEntity = NULL_ENTITY;
            } else {
                saveSceneToPath(m_fileBrowser.result());
            }
        }

        // Preferences window.
        if (m_settingsPanel.onImGuiRender(m_settings, m_showPreferences)) {
            applySettings();
            m_settings.save(kPrefsPath);
        }

        // Auto-save (if enabled, scene has a path, and it's dirty).
        if (m_settings.autoSaveInterval > 0.0f && !m_scenePath.empty() && m_ctx.dirty) {
            m_autoSaveAccum += dt;
            if (m_autoSaveAccum >= m_settings.autoSaveInterval) {
                saveSceneToPath(m_scenePath);
                m_autoSaveAccum = 0.0f;
            }
        } else {
            m_autoSaveAccum = 0.0f;
        }

        handlePrefabRequests();
        handleShortcuts();
        endDockspace();
        updateTitle();
        m_imgui.end(*m_window);

        m_window->swapBuffers();
        Profiler::instance().endFrame();
    }
}

void EditorApplication::beginDockspace() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("##EditorDockHost", nullptr, flags);
    ImGui::PopStyleVar(3);

    ImGui::DockSpace(ImGui::GetID("EditorDockSpace"), ImVec2(0.0f, 0.0f),
                     ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N"))  newScene();
            if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                m_fileBrowser.open(FileBrowser::Mode::Open, "assets", ".escene");
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) saveOrPrompt();
            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
                m_fileBrowser.open(FileBrowser::Mode::Save, "assets", ".escene");
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) m_shouldClose = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Preferences...")) m_showPreferences = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Grid",          nullptr, &m_ctx.overlays.grid);
            ImGui::MenuItem("Show Entity Names",  nullptr, &m_ctx.overlays.names);
            ImGui::MenuItem("Show Colliders",     nullptr, &m_ctx.overlays.colliders);
            ImGui::MenuItem("Show Camera Frustums", nullptr, &m_ctx.overlays.frustums);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Reset Layout", nullptr, false, false);
            ImGui::EndMenu();
        }

        // Play / Pause / Stop toolbar (right of the menus).
        ImGui::Separator();
        const bool playing = m_play.isPlaying();
        if (!playing) {
            if (ImGui::SmallButton("Play")) togglePlay();
        } else {
            if (m_play.isPaused()) { if (ImGui::SmallButton("Resume")) m_play.resume(); }
            else                   { if (ImGui::SmallButton("Pause"))  m_play.pause(); }
            ImGui::SameLine();
            if (ImGui::SmallButton("Stop")) {
                m_play.stop(*m_scene);
                m_ctx.selection.clear();
            }
        }
        ImGui::SameLine();
        ImGui::TextDisabled(playing ? (m_play.isPaused() ? "[PAUSED]" : "[PLAYING]") : "[EDIT]");

        ImGui::EndMenuBar();
    }
}

void EditorApplication::togglePlay() {
    if (m_play.isPlaying()) { m_play.stop(*m_scene); m_ctx.selection.clear(); }
    else                    { m_play.play(*m_scene); }
}

void EditorApplication::endDockspace() {
    ImGui::End();
}

void EditorApplication::drawPanels(f32 dt) {
    m_viewport.onImGuiRender(m_ctx, dt, m_play.isPlaying());
    m_hierarchy.onImGuiRender(m_ctx);
    m_inspector.onImGuiRender(m_ctx);
    m_assetBrowser.onImGuiRender(m_ctx);
    m_profiler.onImGuiRender(m_ctx, dt);
    m_tilemapEditor.onImGuiRender(m_ctx);
}

void EditorApplication::handleShortcuts() {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;   // don't steal keys from text fields

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_P, false)) { togglePlay(); return; }

    // Delete the whole selection as one undo step.
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && !m_ctx.selection.empty()) {
        std::vector<std::unique_ptr<Command>> cmds;
        for (Entity e : m_ctx.selection.entities()) cmds.push_back(Commands::remove(*m_scene, e));
        m_history.push(Commands::composite("Delete selection", std::move(cmds)));
        m_ctx.selection.clear();
        m_ctx.dirty = true;
        return;
    }

    if (!io.KeyCtrl) return;

    const bool z = ImGui::IsKeyPressed(ImGuiKey_Z, false);
    const bool y = ImGui::IsKeyPressed(ImGuiKey_Y, false);
    const bool s = ImGui::IsKeyPressed(ImGuiKey_S, false);
    const bool d = ImGui::IsKeyPressed(ImGuiKey_D, false);
    const bool n = ImGui::IsKeyPressed(ImGuiKey_N, false);
    const bool o = ImGui::IsKeyPressed(ImGuiKey_O, false);

    if (z && io.KeyShift)      m_history.redo();
    else if (z)                m_history.undo();
    else if (y)                m_history.redo();
    else if (s && io.KeyShift) m_fileBrowser.open(FileBrowser::Mode::Save, "assets", ".escene");
    else if (s)                saveOrPrompt();
    else if (n)                newScene();
    else if (o)                m_fileBrowser.open(FileBrowser::Mode::Open, "assets", ".escene");
    else if (d && !m_ctx.selection.empty()) {
        std::vector<std::unique_ptr<Command>> cmds;
        std::vector<std::shared_ptr<Entity>> created;
        for (Entity e : m_ctx.selection.entities()) {
            auto c = std::make_shared<Entity>(NULL_ENTITY);
            created.push_back(c);
            cmds.push_back(Commands::duplicate(*m_scene, e, c));
        }
        m_history.push(Commands::composite("Duplicate selection", std::move(cmds)));
        m_ctx.selection.clear();
        for (auto& c : created) if (*c != NULL_ENTITY) m_ctx.selection.add(*c);
        m_ctx.dirty = true;
    }
}

void EditorApplication::newScene() {
    m_scene = std::make_unique<Scene>("untitled");
    m_ctx.scene    = m_scene.get();
    m_ctx.selection.clear();
    m_ctx.dirty    = false;
    m_history.clear();
    m_scenePath.clear();
}

void EditorApplication::openSceneFile(const std::string& path) {
    auto loaded = std::make_unique<Scene>("untitled");
    auto res = YAMLDeserializer::loadFromFile(path, *loaded);
    if (!res) {
        EMBER_LOG_ERROR("Open scene failed: {}", res.error());
        return;
    }
    m_scene = std::move(loaded);
    m_ctx.scene    = m_scene.get();
    m_ctx.selection.clear();
    m_ctx.dirty    = false;
    m_history.clear();
    m_scenePath = path;
    EMBER_LOG_INFO("Opened scene: {}", path);
}

void EditorApplication::saveSceneToPath(const std::string& path) {
    auto res = YAMLSerializer::saveToFile(*m_scene, path);
    if (!res) {
        EMBER_LOG_ERROR("Save scene failed: {}", res.error());
        return;
    }
    m_scenePath = path;
    m_ctx.dirty = false;
    EMBER_LOG_INFO("Saved scene: {}", path);
}

void EditorApplication::saveOrPrompt() {
    if (m_scenePath.empty())
        m_fileBrowser.open(FileBrowser::Mode::Save, "assets", ".escene");
    else
        saveSceneToPath(m_scenePath);
}

void EditorApplication::handlePrefabRequests() {
    if (m_ctx.savePrefabRequest != NULL_ENTITY) {
        m_pendingPrefabEntity = m_ctx.savePrefabRequest;
        m_ctx.savePrefabRequest = NULL_ENTITY;
        m_fileBrowser.open(FileBrowser::Mode::Save, "assets", ".eprefab");
    }
    if (!m_ctx.openPrefabRequest.empty()) {
        openPrefab(m_ctx.openPrefabRequest);
        m_ctx.openPrefabRequest.clear();
    }
    if (m_ctx.revertPrefabRequest != NULL_ENTITY) {
        revertPrefab(m_ctx.revertPrefabRequest);
        m_ctx.revertPrefabRequest = NULL_ENTITY;
    }
}

void EditorApplication::openPrefab(const std::string& path) {
    std::ifstream f(path);
    if (!f) { EMBER_LOG_ERROR("Open prefab failed: {}", path); return; }
    std::stringstream ss; ss << f.rdbuf();
    auto res = YAMLDeserializer::instantiatePrefab(*m_scene, ss.str(), glm::vec3(0.0f));
    if (!res) { EMBER_LOG_ERROR("Instantiate prefab failed: {}", res.error()); return; }
    const Entity e = res.value();
    World& w = m_scene->world();
    PrefabInstance* pi = w.tryGet<PrefabInstance>(e);
    if (!pi) pi = &w.emplace<PrefabInstance>(e);
    pi->source = path;
    m_ctx.selection.selectOnly(e);
    m_ctx.dirty = true;
}

void EditorApplication::revertPrefab(Entity instance) {
    const PrefabInstance* pi = m_scene->world().tryGet<PrefabInstance>(instance);
    if (!pi) return;
    const std::string path = pi->source;
    glm::vec3 pos{0.0f};
    if (const Transform* t = m_scene->world().tryGet<Transform>(instance)) pos = t->position;

    std::ifstream f(path);
    if (!f) { EMBER_LOG_ERROR("Revert prefab failed (open): {}", path); return; }
    std::stringstream ss; ss << f.rdbuf();
    auto res = YAMLDeserializer::instantiatePrefab(*m_scene, ss.str(), pos);
    if (!res) { EMBER_LOG_ERROR("Revert prefab failed: {}", res.error()); return; }

    const Entity fresh = res.value();
    World& w = m_scene->world();
    PrefabInstance* pi2 = w.tryGet<PrefabInstance>(fresh);
    if (!pi2) pi2 = &w.emplace<PrefabInstance>(fresh);
    pi2->source = path;
    m_scene->destroy(instance);
    m_ctx.selection.selectOnly(fresh);
    m_ctx.dirty = true;
}

void EditorApplication::applySettings() {
    m_imgui.applyTheme(m_settings.theme);
    m_imgui.setUiScale(m_settings.uiScale);
    m_viewport.setSnap(m_settings.gridSnap, m_settings.rotateSnap);
    m_viewport.setCameraSpeeds(m_settings.cameraPanSpeed, m_settings.cameraZoomSpeed);
}

void EditorApplication::updateTitle() {
    std::string name = m_scenePath.empty()
                     ? std::string("untitled")
                     : std::filesystem::path(m_scenePath).filename().string();
    std::string title = "Ember Editor — " + name + (m_ctx.dirty ? " *" : "");
    if (title != m_lastTitle) {
        m_window->setTitle(title);
        m_lastTitle = title;
    }
}

} // namespace ember
