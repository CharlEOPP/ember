#pragma once
#include "ember/core/Types.h"
#include "ember/platform/Event.h"
#include <functional>
#include <string>

// Forward-declare GLFW handle — never expose GLFW types in downstream headers
struct GLFWwindow;

namespace ember {

struct WindowSpec {
    std::string title      = "Ember";
    u32         width      = 1280;
    u32         height     = 720;
    bool        vsync      = true;
    bool        fullscreen = false;
};

class Window {
public:
    using EventCallbackFn = std::function<void(platform::Event&)>;

    explicit Window(const WindowSpec& spec);
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&)                 = delete;
    Window& operator=(Window&&)      = delete;

    void pollEvents();
    void swapBuffers();
    bool shouldClose() const;

    void setEventCallback(EventCallbackFn fn);
    void setVSync(bool enabled);
    void setTitle(const std::string& title);

    u32 getWidth()  const;
    u32 getHeight() const;

    // Exposed for the RHI context setup only — do not use in game code
    GLFWwindow* getNativeHandle() const { return m_window; }

private:
    void init(const WindowSpec& spec);
    void shutdown();

    GLFWwindow*     m_window = nullptr;
    EventCallbackFn m_eventCallback;

    struct State {
        u32  width  = 1280;
        u32  height = 720;
        bool vsync  = true;
    } m_state;
};

} // namespace ember
