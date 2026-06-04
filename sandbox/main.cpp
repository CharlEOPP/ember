#include "ember/core/Core.h"
#include "ember/platform/Window.h"
#include "ember/platform/Timer.h"
#include "ember/renderer/RHI.h"
#include "ember/renderer/Renderer2D.h"
#include "ember/renderer/Camera.h"
#include "ember/renderer/DebugDraw.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
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

int main() {
    Log::init();
    EMBER_LOG_INFO("Ember initialised");

    WindowSpec spec;
    spec.title  = "Ember Sandbox — 2D Renderer";
    spec.width  = 1280;
    spec.height = 720;
    spec.vsync  = true;
    auto window = std::make_unique<Window>(spec);

    RHI::init(RHIBackend::OpenGL);
    RHI::setClearColor(0.07f, 0.07f, 0.10f);
    RHI::setViewport(0, 0, window->getWidth(), window->getHeight());

    Renderer2D::init();
    DebugDraw::init();   // no-op in Release builds

    // Editor camera framing a 32x32 grid centred on the origin.
    EditorCamera camera;
    camera.setViewportSize(window->getWidth(), window->getHeight());
    camera.zoom(5.0f - 18.0f);   // half-height -> ~18 world units

    // A few textures so batching uses multiple slots.
    auto white    = RHI::whiteTexture();
    auto checkerA = makeChecker(8, 0xFFFFFFFFu, 0xFF3060FFu);   // white / red-ish
    auto checkerB = makeChecker(8, 0xFFFFFFFFu, 0xFF60C040u);   // white / green-ish
    const std::shared_ptr<ITexture2D> textures[3] = { white, checkerA, checkerB };

    window->setEventCallback([&](platform::Event& e) {
        if (e.getType() == platform::EventType::WindowResize) {
            auto& re = static_cast<platform::WindowResizeEvent&>(e);
            RHI::setViewport(0, 0, re.width, re.height);
            camera.setViewportSize(re.width, re.height);
        }
    });

    constexpr int kGrid = 32;   // 32 * 32 = 1024 sprites

    Timer frameTimer, fpsTimer;
    u32   frameCount = 0;
    f32   elapsed    = 0.0f;

    while (!window->shouldClose()) {
        const auto dt = static_cast<f32>(frameTimer.elapsed());
        frameTimer.reset();
        elapsed += dt;
        Time::update(dt);

        window->pollEvents();
        RHI::clear();

        Renderer2D::beginScene(camera.viewProjection());
        for (int y = 0; y < kGrid; ++y) {
            for (int x = 0; x < kGrid; ++x) {
                const glm::vec2 pos{ x - kGrid / 2 + 0.5f, y - kGrid / 2 + 0.5f };
                const f32 rot = elapsed + 0.2f * static_cast<f32>(x + y);
                glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(pos, 0.0f))
                            * glm::rotate(glm::mat4(1.0f), rot, glm::vec3(0, 0, 1))
                            * glm::scale(glm::mat4(1.0f), glm::vec3(0.8f));
                const glm::vec4 color{
                    0.5f + 0.5f * std::sin(static_cast<f32>(x) * 0.3f),
                    0.5f + 0.5f * std::cos(static_cast<f32>(y) * 0.3f),
                    0.8f, 1.0f
                };
                Renderer2D::drawQuad(t, color, textures[(x + y) % 3]);
            }
        }
        Renderer2D::endScene();

        // Debug overlay (visible in Debug builds; compiled out in Release).
        DebugDraw::begin(camera.viewProjection());
        DebugDraw::rect(glm::vec2(0.0f), glm::vec2(static_cast<f32>(kGrid)), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        DebugDraw::circle(glm::vec2(0.0f), static_cast<f32>(kGrid) * 0.5f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
        DebugDraw::flush();

        window->swapBuffers();

        ++frameCount;
        if (fpsTimer.elapsed() >= 1.0) {
            const RendererStats& s = Renderer2D::stats();
            window->setTitle(std::format("Ember Sandbox — {} FPS | {} quads | {} draw calls",
                                         frameCount, s.quadCount, s.drawCalls));
            frameCount = 0;
            fpsTimer.reset();
        }
    }

    DebugDraw::shutdown();
    Renderer2D::shutdown();
    EMBER_LOG_INFO("Ember shutting down");
    Log::shutdown();
    return 0;
}
