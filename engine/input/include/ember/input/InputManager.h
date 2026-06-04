#pragma once
#include "ember/input/Keys.h"
#include "ember/input/InputAction.h"
#include <glm/glm.hpp>
#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

// InputManager owns per-frame input state and is the single source of truth the
// free-function `Input::` facade reads from. GLFW is confined to InputManager.cpp:
// callbacks are installed on Window::getNativeHandle() and translated into the
// GLFW-free enums above before touching any state (the `feed*` seam, which tests
// also drive headlessly).

struct GLFWwindow; // forward decl — no GLFW header in this public header

namespace ember {

class Window;
class EventBus;

class InputManager {
public:
    InputManager() = default;
    ~InputManager();

    InputManager(const InputManager&)            = delete;
    InputManager& operator=(const InputManager&) = delete;

    // Installs GLFW callbacks on the window's native handle and makes this the
    // active manager for the Input:: facade. `bus` is optional; when set, raw
    // input is re-broadcast as KeyPressedEvent/MouseMovedEvent/etc.
    void init(Window& window, EventBus* bus = nullptr);
    // Handle-based core; also lets callers wire input without the platform module.
    void initWithHandle(GLFWwindow* handle, EventBus* bus = nullptr);
    void shutdown();

    // Roll current->previous, recompute edges, reset per-frame deltas, poll the
    // gamepad. Call once per frame, before game logic.
    void update();

    // Make this the active manager without a window (test seam / headless use).
    void makeActive();

    static InputManager* active() { return s_active; }

    // ---- Raw state seam: called by GLFW callbacks AND by tests ----
    void feedKey(Key k, bool down);
    void feedMouseButton(Mouse m, bool down);
    void feedMousePosition(f32 x, f32 y);
    void feedScroll(f32 yOffset);

    // ---- Polling API (used by the Input:: facade) ----
    bool isKeyDown(Key k) const;
    bool isKeyPressed(Key k) const;
    bool isKeyReleased(Key k) const;

    bool isMouseButtonDown(Mouse m) const;
    bool isMouseButtonPressed(Mouse m) const;
    bool isMouseButtonReleased(Mouse m) const;

    glm::vec2 mousePosition() const { return m_mousePos; }
    glm::vec2 mouseDelta()    const { return m_mouseDelta; }
    f32       mouseScroll()   const { return m_scroll; }

    // ---- Gamepad ----
    bool isGamepadConnected() const { return m_gamepadConnected; }
    f32  gamepadAxis(GamepadAxis a) const;
    bool isGamepadButtonDown(GamepadButton b) const;

    // ---- Action map ----
    void addAction(const InputAction& action);
    ActionState getAction(const std::string& name) const;
    bool loadActionMap(const std::filesystem::path& path);
    bool saveActionMap(const std::filesystem::path& path) const;
    void loadDefaultActionMap();

    // ---- Context stack: only the top context's actions resolve ----
    void pushContext(const std::string& name);
    void popContext();
    const std::string& activeContext() const;

private:
    static InputManager* s_active;

    GLFWwindow* m_window = nullptr;
    EventBus*   m_bus    = nullptr;

    std::array<bool, kKeyCount>   m_curKeys{};
    std::array<bool, kKeyCount>   m_prevKeys{};
    std::array<bool, kKeyCount>   m_pressedKeys{};   // edge: down this frame
    std::array<bool, kKeyCount>   m_releasedKeys{};  // edge: up this frame
    std::array<bool, kMouseCount> m_curMouse{};
    std::array<bool, kMouseCount> m_prevMouse{};
    std::array<bool, kMouseCount> m_pressedMouse{};
    std::array<bool, kMouseCount> m_releasedMouse{};

    glm::vec2 m_mousePos{0.0f};
    glm::vec2 m_prevMousePos{0.0f};
    glm::vec2 m_mouseDelta{0.0f};
    f32       m_scroll = 0.0f;        // accumulated this frame, reset in update()
    f32       m_scrollAccum = 0.0f;   // accumulates between updates
    bool      m_firstMouse = true;

    // Gamepad snapshot (filled by update()).
    bool m_gamepadConnected = false;
    std::array<bool, static_cast<usize>(GamepadButton::Count)> m_gamepadButtons{};
    std::array<f32,  static_cast<usize>(GamepadAxis::Count)>   m_gamepadAxes{};
    f32  m_gamepadDeadzone = 0.15f;

    // Actions are scoped per context; the context stack selects which fire.
    std::unordered_map<std::string, std::unordered_map<std::string, InputAction>> m_contextActions;
    std::vector<std::string> m_contextStack;

    ActionState evaluate(const InputAction& a) const;
    void pollGamepad();
};

} // namespace ember
