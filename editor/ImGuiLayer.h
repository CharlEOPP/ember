#pragma once

namespace ember {

class Window;

// Owns the Dear ImGui context + GLFW/OpenGL3 backends. begin()/end() bracket the
// per-frame UI; the editor draws its panels between them.
class ImGuiLayer {
public:
    void init(Window& window);
    void begin();                 // new ImGui frame
    void end(Window& window);     // render draw data (+ multi-viewport update)
    void shutdown();

    [[nodiscard]] bool initialized() const { return m_initialized; }

private:
    bool m_initialized = false;
};

} // namespace ember
