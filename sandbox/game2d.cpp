// Epic 12 sandbox slice — exercises the 2D-gameplay systems together:
// custom 2D physics (gravity + AABB resolution), the tilemap renderer, the
// particle system, sprite rendering with WorldTransform sync, a runtime UI HUD,
// and the audio facade. Asset-free: textures/fonts are optional, so every system
// runs from procedural data. Build target: ember_sandbox_game2d.
#include "ember/core/Core.h"
#include "ember/platform/Window.h"
#include "ember/platform/Timer.h"
#include "ember/renderer/RHI.h"
#include "ember/renderer/Renderer2D.h"
#include "ember/renderer/Camera.h"
#include "ember/renderer/DebugDraw.h"
#include "ember/renderer/Components2D.h"
#include "ember/renderer/SpriteRenderSystem.h"
#include "ember/renderer/Tilemap.h"
#include "ember/renderer/TilemapRenderSystem.h"
#include "ember/renderer/ParticleEmitter.h"
#include "ember/renderer/ParticleSystem.h"
#include "ember/renderer/UIComponents.h"
#include "ember/renderer/UISystem.h"
#include "ember/ecs/Components.h"
#include "ember/scene/Scene.h"
#include "ember/scene/TransformSystem.h"
#include "ember/input/InputManager.h"
#include "ember/assets/AssetManager.h"
#include "ember/physics2d/RigidBody2D.h"
#include "ember/physics2d/Colliders2D.h"
#include "ember/physics2d/Physics2DSystem.h"
#include "ember/audio/Audio.h"
#include "ember/audio/AudioEngine.h"

#include <glm/glm.hpp>
#include <format>
#include <memory>

using namespace ember;

static Entity makeBox(Scene& scene, glm::vec2 pos, glm::vec2 half,
                      BodyType2D type, const glm::vec4& color) {
    Entity e = scene.create();
    scene.world().get<Transform>(e).position = glm::vec3(pos, 0.0f);
    scene.world().get<Transform>(e).scale    = glm::vec3(half * 2.0f, 1.0f);
    auto& rb = scene.world().emplace<RigidBody2D>(e);
    rb.type  = type;
    auto& bc = scene.world().emplace<BoxCollider2D>(e);
    bc.halfExtents = half;
    auto& sr = scene.world().emplace<SpriteRenderer>(e);
    sr.color = color;
    return e;
}

int main() {
    Log::init();
    EMBER_LOG_INFO("Ember 2D gameplay slice");

    WindowSpec spec;
    spec.title = "Ember — 2D Gameplay Slice";
    spec.width = 1280; spec.height = 720; spec.vsync = true;
    auto window = std::make_unique<Window>(spec);

    RHI::init(RHIBackend::OpenGL);
    RHI::setClearColor(0.06f, 0.07f, 0.10f);
    RHI::setViewport(0, 0, window->getWidth(), window->getHeight());
    Renderer2D::init();
    DebugDraw::init();

    EditorCamera camera;
    camera.setViewportSize(window->getWidth(), window->getHeight());
    camera.zoom(5.0f - 12.0f);   // ~12 world-units half-height

    AssetManager assets;
    AudioEngine  audioEngine;
    Audio::init(audioEngine, &assets);   // no-op without EMBER_ENABLE_AUDIO / clips

    InputManager input;
    input.init(*window);
    input.loadDefaultActionMap();

    // ---- Systems ----
    Scene scene("game2d");
    World& w = scene.world();

    TransformSystem      transforms;
    Physics2DSystem      physics;
    physics.setGravity({0.0f, -20.0f});
    SpriteRenderSystem   sprites(assets);
    TilemapRenderSystem  tiles(assets);
    ParticleSystem       particles;
    UISystem             ui(assets);
    ui.setScreenSize({static_cast<f32>(window->getWidth()), static_cast<f32>(window->getHeight())});

    window->setEventCallback([&](platform::Event& e) {
        if (e.getType() == platform::EventType::WindowResize) {
            auto& re = static_cast<platform::WindowResizeEvent&>(e);
            RHI::setViewport(0, 0, re.width, re.height);
            camera.setViewportSize(re.width, re.height);
            ui.setScreenSize({static_cast<f32>(re.width), static_cast<f32>(re.height)});
        }
    });

    // ---- World content ----
    makeBox(scene, {0.0f, -6.0f}, {10.0f, 0.5f}, BodyType2D::Static,  {0.30f, 0.32f, 0.40f, 1.0f}); // floor
    Entity player = makeBox(scene, {0.0f, 6.0f}, {0.5f, 0.5f}, BodyType2D::Dynamic, {1.0f, 0.85f, 0.2f, 1.0f});
    makeBox(scene, {1.4f, 9.0f}, {0.5f, 0.5f}, BodyType2D::Dynamic, {0.4f, 0.8f, 1.0f, 1.0f});

    // Particle fountain.
    Entity fx = scene.create("FX");
    w.get<Transform>(fx).position = {-4.0f, -5.0f, 0.0f};
    { auto& pe = w.emplace<ParticleEmitter>(fx);
      pe.emitRate = 40.0f; pe.gravity = {0.0f, 3.0f};
      pe.velMin = {-1.5f, 2.0f}; pe.velMax = {1.5f, 5.0f};
      pe.startColor = {1.0f, 0.6f, 0.1f, 1.0f}; pe.endColor = {1.0f, 0.1f, 0.0f, 0.0f}; }

    // Tilemap strip (renders as flat quads without a tileset texture).
    Entity map = scene.create("Map");
    w.get<Transform>(map).position = {-9.0f, -5.5f, 0.0f};
    { auto& tm = w.emplace<Tilemap>(map);
      tm.width = 6; tm.height = 1; tm.tileWorldSize = 1.0f;
      tm.tiles = {1, 1, 0, 1, 1, 1}; }

    // UI HUD: a panel + a button.
    Entity panel = scene.create("Panel");
    { auto& img = w.emplace<UIImage>(panel);
      img.rect.anchor = UIAnchor::TopLeft; img.rect.offset = {16.0f, 16.0f};
      img.rect.size = {220.0f, 56.0f}; img.color = {0.0f, 0.0f, 0.0f, 0.5f}; img.order = 0; }
    Entity button = scene.create("Button");
    { auto& btn = w.emplace<UIButton>(button);
      btn.rect.anchor = UIAnchor::TopLeft; btn.rect.offset = {28.0f, 28.0f};
      btn.rect.size = {180.0f, 32.0f}; btn.order = 1; }

    Timer frameTimer, fpsTimer;
    u32 frameCount = 0;

    while (!window->shouldClose()) {
        const auto dt = static_cast<f32>(frameTimer.elapsed());
        frameTimer.reset();
        Time::update(dt);

        window->pollEvents();
        input.update();

        // --- simulate ---
        physics.update(w, dt);                       // moves Transforms of dynamic bodies
        for (auto [e, wt] : w.view<WorldTransform>()) // physics bypasses the dirty flag
            wt.dirty = true;
        transforms.update(w, dt);                    // rebuild WorldTransforms for the sprite pass

        // --- render world ---
        RHI::clear();
        Renderer2D::beginScene(camera.viewProjection());
        tiles.update(w, dt);
        sprites.update(w, dt);
        particles.update(w, dt);
        Renderer2D::endScene();

        // --- render UI (own screen-space scene) ---
        ui.update(w, dt);
        if (w.get<UIButton>(button).clicked) {
            Audio::playOneShot({}, 1.0f);            // null-safe; plays when a clip/device exist
            EMBER_LOG_INFO("HUD button clicked");
        }

        window->swapBuffers();

        if (++frameCount, fpsTimer.elapsed() >= 1.0) {
            window->setTitle(std::format("Ember — 2D Gameplay Slice — {} FPS | player y={:.2f}",
                                         frameCount, w.get<Transform>(player).position.y));
            frameCount = 0; fpsTimer.reset();
        }
    }

    Audio::shutdown();
    input.shutdown();
    DebugDraw::shutdown();
    Renderer2D::shutdown();
    Log::shutdown();
    return 0;
}
