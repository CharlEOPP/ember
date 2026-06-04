#pragma once
#include "ember/core/Types.h"
#include "ember/physics2d/Physics2DSystem.h"

#include <memory>
#include <string>

namespace ember {

class Scene;
class AssetManager;
class ScriptSystem;
class SpriteAnimationSystem;

// Drives the Edit ⇄ Play ⇄ Pause state machine. play() snapshots the scene to a
// YAML string; stop() restores it. While Playing it advances the simulation
// systems that have no GL dependency (physics, scripts, sprite animation); the
// ViewportPanel keeps rendering (and steps particles with the real dt).
class PlayModeController {
public:
    enum class Mode { Edit, Playing, Paused };

    explicit PlayModeController(AssetManager& assets);
    ~PlayModeController();   // both defined in .cpp where the system types are complete

    [[nodiscard]] Mode mode() const { return m_mode; }
    [[nodiscard]] bool isPlaying() const { return m_mode != Mode::Edit; }
    [[nodiscard]] bool isPaused()  const { return m_mode == Mode::Paused; }

    void play(Scene& scene);
    void pause() { if (m_mode == Mode::Playing) m_mode = Mode::Paused; }
    void resume() { if (m_mode == Mode::Paused) m_mode = Mode::Playing; }
    void stop(Scene& scene);

    void update(Scene& scene, f32 dt);   // advances sim while Playing

private:
    AssetManager* m_assets = nullptr;
    Mode          m_mode   = Mode::Edit;
    std::string   m_snapshot;

    std::unique_ptr<ScriptSystem>         m_scripts;
    std::unique_ptr<SpriteAnimationSystem> m_anim;
    Physics2DSystem                       m_physics;
};

} // namespace ember
