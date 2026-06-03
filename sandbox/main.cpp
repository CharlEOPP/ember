#include "ember/core/Core.h"
#include "ember/platform/Window.h"
#include "ember/platform/Timer.h"
#include "ember/renderer/RHI.h"

#include <format>
#include <memory>

int main() {
    ember::Log::init();
    EMBER_LOG_INFO("Ember initialised");

    // ---- Window ----
    ember::WindowSpec spec;
    spec.title  = "Ember Sandbox";
    spec.width  = 1280;
    spec.height = 720;
    spec.vsync  = true;

    auto window = std::make_unique<ember::Window>(spec);

    // ---- OpenGL context ----
    ember::RHI::init(ember::RHIBackend::OpenGL);
    ember::RHI::setClearColor(0.1f, 0.1f, 0.15f);
    // Use the real framebuffer size (pixels) — correct on HiDPI / scaled displays.
    ember::RHI::setViewport(0, 0, window->getWidth(), window->getHeight());

    // ---- Hard-coded triangle (NDC) ----
    // Vertex layout: position only (x, y, z)
    const float vertices[] = {
        -0.5f, -0.5f, 0.0f,   // bottom-left
         0.5f, -0.5f, 0.0f,   // bottom-right
         0.0f,  0.5f, 0.0f    // top-centre
    };
    const ember::u32 indices[] = { 0, 1, 2 };

    auto vbo = ember::RHI::createVertexBuffer(vertices, sizeof(vertices),
                                               ember::BufferUsage::Static);
    auto ibo = ember::RHI::createIndexBuffer(indices, 3);

    // GLSL 410 — compatible with OpenGL 4.1 (macOS) and 4.x (Windows/Linux)
    const char* vertSrc = R"glsl(
        #version 410 core
        layout(location = 0) in vec3 a_Position;
        void main() {
            gl_Position = vec4(a_Position, 1.0);
        }
    )glsl";

    const char* fragSrc = R"glsl(
        #version 410 core
        out vec4 fragColor;
        void main() {
            fragColor = vec4(1.0, 1.0, 1.0, 1.0);  // white
        }
    )glsl";

    auto shader = ember::RHI::createShader(vertSrc, fragSrc);
    EMBER_VERIFY(shader != nullptr, "Failed to compile sandbox shader");

    // ---- Event callback ----
    window->setEventCallback([&](ember::Event& e) {
        if (e.getType() == ember::EventType::WindowResize) {
            auto& re = static_cast<ember::WindowResizeEvent&>(e);
            ember::RHI::setViewport(0, 0, re.width, re.height);
            EMBER_LOG_DEBUG("Window resized: {}x{}", re.width, re.height);
        }
    });

    // ---- Main loop ----
    ember::Timer frameTimer;
    ember::Timer fpsTimer;
    ember::u32   frameCount = 0;

    while (!window->shouldClose()) {
        const auto dt = static_cast<ember::f32>(frameTimer.elapsed());
        frameTimer.reset();
        ember::Time::update(dt);

        window->pollEvents();

        ember::RHI::clear();
        shader->bind();
        ember::RHI::drawIndexed(vbo, ibo, 3);

        window->swapBuffers();

        ++frameCount;
        if (fpsTimer.elapsed() >= 1.0) {
            const ember::u32 fps = frameCount;
            window->setTitle(std::format("Ember Sandbox — {} FPS", fps));
            frameCount = 0;
            fpsTimer.reset();
        }
    }

    EMBER_LOG_INFO("Ember shutting down");
    ember::Log::shutdown();
    return 0;
}
