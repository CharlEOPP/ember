#include "ember/core/Core.h"
#include "ember/platform/Window.h"
#include "ember/platform/Timer.h"
#include "ember/renderer/RHI.h"
#include "ember/renderer/Renderer2D.h"
#include "ember/renderer/Camera.h"
#include "ember/renderer/DebugDraw.h"
#include "ember/ecs/Components.h"
#include "ember/scene/Scene.h"
#include "ember/input/InputManager.h"
#include "ember/scripting/ScriptSystem.h"

#include "scripts/Spinner.h"
#include "scripts/PlayerController.h"
#include "scripts/Lifetime.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdlib>
#include <format>
#include <memory>
#include <vector>

using namespace ember;

// Build a small procedural checker texture (no asset files needed for the demo).
static std::shared_ptr<ITexture2D> makeChecker(u32 size, u32 colorA, u32 colorB) {
    std::vector<u32> pixels(static_cast<usize>(size) * size);
    const u32 half = size / 2;
    for (u32 y = 0; y < size; ++y)
        for (u32 x = 0; x < size; ++x)
            pixels[y * size + x] = (((x / half) + (y / half)) % 2 == 0) ? colorA : colorB;

    TextureSpec spec;
    spec.width  = size;
    spec.height = size;
    spec.format = TextureFormat::RGBA8;
    spec.filter = TextureFilter::Nearest;
    return RHI::createTexture2D(spec, pixels.data());
}

static glm::mat4 trsMatrix(const Transform& t) {
    return glm::translate(glm::mat4(1.0f), t.position)
         * glm::rotate(glm::mat4(1.0f), t.rotation, glm::vec3(0, 0, 1))
         * glm::scale(glm::mat4(1.0f), t.scale);
}

int main() {
    Log::init();
    EMBER_LOG_INFO("Ember initialised");

    WindowSpec spec;
    spec.title  = "Ember Sandbox — Scripting";
    spec.width  = 1280;
    spec.height = 720;
    spec.vsync  = true;
    auto window = std::make_unique<Window>(spec);

    RHI::init(RHIBackend::OpenGL);
    RHI::setClearColor(0.07f, 0.07f, 0.10f);
    RHI::setViewport(0, 0, window->getWidth(), window->getHeight());

    Renderer2D::init();
    DebugDraw::init();   // no-op in Release builds

    EditorCamera camera;
    camera.setViewportSize(window->getWidth(), window->getHeight());
    camera.zoom(5.0f - 18.0f);   // half-height -> ~18 world units

    auto white    = RHI::whiteTexture();
    auto checkerA = makeChecker(8, 0xFFFFFFFFu, 0xFF3060FFu);
    auto checkerB = makeChecker(8, 0xFFFFFFFFu, 0xFF60C040u);

    // ---- Input: drives PlayerController via the Epic 05 facade ----
    InputManager input;
    input.init(*window);
    input.loadDefaultActionMap();   // "game" context: MoveX/MoveY/Jump

    window->setEventCallback([&](platform::Event& e) {
        if (e.getType() == platform::EventType::WindowResize) {
            auto& re = static_cast<platform::WindowResizeEvent&>(e);
            RHI::setViewport(0, 0, re.width, re.height);
            camera.setViewportSize(re.width, re.height);
        }
    });

    // ---- Scene + scripts ----
    Scene scene("sandbox");
    ScriptSystem scripts(scene);

    // A grid of independently-rotating Spinners (validates throughput, SBX-03).
    constexpr int kGrid = 22;   // 22*22 = 484 spinners
    for (int y = 0; y < kGrid; ++y) {
        for (int x = 0; x < kGrid; ++x) {
            Entity e = scene.create();
            Transform& t = scene.world().get<Transform>(e);
            t.position = { x - kGrid / 2 + 0.5f, y - kGrid / 2 + 0.5f, 0.0f };
            t.scale    = glm::vec3(0.7f);
            auto& sp = scene.world().emplace<game::Spinner>(e);
            sp.speed = 0.5f + 2.0f * (static_cast<float>(std::rand()) / RAND_MAX);
        }
    }

    // The player: WASD/arrows move it; Space "jumps" (SBX-01).
    Entity player = scene.create("Player");
    scene.world().get<Transform>(player).scale = glm::vec3(1.4f);
    scene.world().emplace<game::PlayerController>(player).speed = 8.0f;

    Timer frameTimer, fpsTimer;
    u32   frameCount = 0;

    while (!window->shouldClose()) {
        const auto dt = static_cast<f32>(frameTimer.elapsed());
        frameTimer.reset();
        Time::update(dt);

        window->pollEvents();
        input.update();                       // roll input edges (after poll)
        scripts.update(scene.world(), dt);    // run all script lifecycle hooks

        RHI::clear();
        Renderer2D::beginScene(camera.viewProjection());
        for (auto&& [e, t] : scene.world().view<Transform>()) {
            const bool isPlayer = scene.world().has<game::PlayerController>(e);
            const glm::vec4 color = isPlayer ? glm::vec4(1.0f, 0.85f, 0.2f, 1.0f)
                                             : glm::vec4(0.5f, 0.6f, 0.9f, 1.0f);
            Renderer2D::drawQuad(trsMatrix(t), color, isPlayer ? checkerA : checkerB);
        }
        Renderer2D::endScene();

        window->swapBuffers();

        ++frameCount;
        if (fpsTimer.elapsed() >= 1.0) {
            const RendererStats& s = Renderer2D::stats();
            window->setTitle(std::format("Ember Sandbox — Scripting — {} FPS | {} quads",
                                         frameCount, s.quadCount));
            frameCount = 0;
            fpsTimer.reset();
        }
    }

    input.shutdown();
    DebugDraw::shutdown();
    Renderer2D::shutdown();
    EMBER_LOG_INFO("Ember shutting down");
    Log::shutdown();
    return 0;
}
