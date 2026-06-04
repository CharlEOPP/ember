#include "ember/platform/Window.h"
#include "ember/core/Log.h"
#include "ember/core/Assert.h"

#include <GLFW/glfw3.h>
#include <stdexcept>

namespace ember {

static u32 s_glfwWindowCount = 0;

Window::Window(const WindowSpec& spec) {
    init(spec);
}

Window::~Window() {
    shutdown();
}

void Window::init(const WindowSpec& spec) {
    m_state.width  = spec.width;
    m_state.height = spec.height;
    m_state.vsync  = spec.vsync;

    if (s_glfwWindowCount == 0) {
        const int ok = glfwInit();
        EMBER_VERIFY(ok, "Failed to initialise GLFW");

        glfwSetErrorCallback([](int error, const char* desc) {
            EMBER_LOG_ERROR("GLFW error {}: {}", error, desc);
        });
    }

    // Request OpenGL core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(EMBER_PLATFORM_MACOS)
    // macOS caps OpenGL at 4.1 core and has no KHR_debug.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    // 4.3 makes KHR_debug part of core, so glDebugMessageCallback works.
    // (On a 4.1 context it's only an extension and enabling it errors.)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    #if !defined(NDEBUG)
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    #endif
#endif

    m_window = glfwCreateWindow(
        static_cast<int>(spec.width),
        static_cast<int>(spec.height),
        spec.title.c_str(),
        spec.fullscreen ? glfwGetPrimaryMonitor() : nullptr,
        nullptr
    );

    if (!m_window)
        throw std::runtime_error("Failed to create GLFW window");

    ++s_glfwWindowCount;
    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    setVSync(spec.vsync);

    // Store the real framebuffer size in PIXELS (may differ from the requested
    // size on HiDPI / scaled displays). The viewport must use pixels, not the
    // window's screen-coordinate size.
    {
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(m_window, &fbW, &fbH);
        m_state.width  = static_cast<u32>(fbW);
        m_state.height = static_cast<u32>(fbH);
    }

    // ---- GLFW callbacks ----
    // Framebuffer-size (pixels), NOT window-size (screen coords) — correct on HiDPI.
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* w, int width, int height) {
        auto& self = *static_cast<Window*>(glfwGetWindowUserPointer(w));
        self.m_state.width  = static_cast<u32>(width);
        self.m_state.height = static_cast<u32>(height);
        if (self.m_eventCallback) {
            platform::WindowResizeEvent e(static_cast<u32>(width), static_cast<u32>(height));
            self.m_eventCallback(e);
        }
    });

    glfwSetWindowCloseCallback(m_window, [](GLFWwindow* w) {
        auto& self = *static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (self.m_eventCallback) {
            platform::WindowCloseEvent e;
            self.m_eventCallback(e);
        }
    });

    EMBER_LOG_INFO("Window created: {}x{} '{}'", spec.width, spec.height, spec.title);
}

void Window::shutdown() {
    glfwDestroyWindow(m_window);
    m_window = nullptr;
    --s_glfwWindowCount;
    if (s_glfwWindowCount == 0) {
        EMBER_LOG_DEBUG("All windows destroyed — terminating GLFW");
        glfwTerminate();
    }
}

void Window::pollEvents()  { glfwPollEvents(); }
void Window::swapBuffers() { glfwSwapBuffers(m_window); }

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window) != 0;
}

void Window::setEventCallback(EventCallbackFn fn) {
    m_eventCallback = std::move(fn);
}

void Window::setVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
    m_state.vsync = enabled;
}

void Window::setTitle(const std::string& title) {
    glfwSetWindowTitle(m_window, title.c_str());
}

u32 Window::getWidth()  const { return m_state.width;  }
u32 Window::getHeight() const { return m_state.height; }

} // namespace ember
