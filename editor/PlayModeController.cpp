#include "PlayModeController.h"

#include "ember/scene/Scene.h"
#include "ember/ecs/World.h"
#include "ember/scripting/ScriptSystem.h"
#include "ember/renderer/SpriteAnimationSystem.h"
#include "ember/serialization/YAMLSerializer.h"
#include "ember/core/Log.h"
#include "ember/core/Profiler.h"

namespace ember {

PlayModeController::PlayModeController(AssetManager& assets) : m_assets(&assets) {}
PlayModeController::~PlayModeController() = default;

void PlayModeController::play(Scene& scene) {
    if (m_mode != Mode::Edit) return;

    m_snapshot = YAMLSerializer::serialize(scene);   // restore point

    m_scripts = std::make_unique<ScriptSystem>(scene);
    m_scripts->setAssetManager(m_assets);
    m_anim    = std::make_unique<SpriteAnimationSystem>(*m_assets);
    m_physics.setScriptSystem(m_scripts.get());
    m_physics.setGravity({0.0f, -20.0f});

    m_mode = Mode::Playing;
    EMBER_LOG_INFO("Play mode: started");
}

void PlayModeController::stop(Scene& scene) {
    if (m_mode == Mode::Edit) return;

    scene.world().clear();
    auto res = YAMLDeserializer::deserialize(m_snapshot, scene);
    if (!res) EMBER_LOG_ERROR("Play stop: restore failed: {}", res.error());

    m_scripts.reset();
    m_anim.reset();
    m_mode = Mode::Edit;
    EMBER_LOG_INFO("Play mode: stopped (scene restored)");
}

void PlayModeController::update(Scene& scene, f32 dt) {
    if (m_mode != Mode::Playing) return;   // frozen while Paused / Edit
    World& w = scene.world();
    { EMBER_PROFILE_SCOPE("Physics");   m_physics.update(w, dt); }
    if (m_scripts) { EMBER_PROFILE_SCOPE("Scripts");   m_scripts->update(w, dt); }
    if (m_anim)    { EMBER_PROFILE_SCOPE("Animation"); m_anim->update(w, dt); }
    // Particles are stepped + drawn by the ViewportPanel inside beginScene.
}

} // namespace ember
