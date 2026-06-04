#include "ember/input/InputManager.h"
#include "ember/input/Input.h"
#include "ember/platform/Window.h"
#include "ember/events/EventBus.h"
#include "ember/events/Events.h"
#include "ember/core/Log.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE   // we don't need any GL headers here (build usually sets this)
#endif
#include <GLFW/glfw3.h>

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>
#include <fstream>

namespace ember {

InputManager* InputManager::s_active = nullptr;

InputManager::~InputManager() {
    if (s_active == this) shutdown();
}

// ---------------------------------------------------------------------------
// GLFW translation (the only place GLFW codes are touched)
// ---------------------------------------------------------------------------
namespace {

Key translateKey(int glfwKey) {
    // Ember's Key values are numerically identical to GLFW keycodes by design;
    // bounds-check and cast, mapping anything out of range to Unknown.
    if (glfwKey < 0 || static_cast<usize>(glfwKey) >= kKeyCount) return Key::Unknown;
    return static_cast<Key>(glfwKey);
}

Mouse translateMouse(int glfwButton) {
    if (glfwButton < 0 || static_cast<usize>(glfwButton) >= kMouseCount) return Mouse::Count;
    return static_cast<Mouse>(glfwButton);
}

InputAction::BindingKind kindFromString(const std::string& s) {
    if (s == "mouse")         return InputAction::BindingKind::MouseButton;
    if (s == "gamepadButton") return InputAction::BindingKind::GamepadButton;
    if (s == "axis")          return InputAction::BindingKind::GamepadAxis;
    return InputAction::BindingKind::Key;
}

const char* kindToString(InputAction::BindingKind k) {
    switch (k) {
        case InputAction::BindingKind::MouseButton:   return "mouse";
        case InputAction::BindingKind::GamepadButton: return "gamepadButton";
        case InputAction::BindingKind::GamepadAxis:   return "axis";
        case InputAction::BindingKind::Key:           return "key";
    }
    return "key";
}

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
void InputManager::makeActive() { s_active = this; }

void InputManager::init(Window& window, EventBus* bus) {
    initWithHandle(window.getNativeHandle(), bus);
}

void InputManager::initWithHandle(GLFWwindow* handle, EventBus* bus) {
    m_window = handle;
    m_bus    = bus;
    s_active = this;

    if (!m_window) {
        EMBER_LOG_WARN("InputManager::init called with a null native window handle");
        return;
    }

    // NOTE: the Window owns the GLFW user pointer (for its resize/close
    // callbacks), so input callbacks route through s_active instead.
    glfwSetKeyCallback(m_window, [](GLFWwindow*, int key, int /*sc*/, int action, int mods) {
        if (!s_active || action == GLFW_REPEAT) {
            if (s_active && s_active->m_bus && action == GLFW_REPEAT)
                s_active->m_bus->post<KeyPressedEvent>(key, mods, true);
            return;
        }
        const bool down = (action == GLFW_PRESS);
        s_active->feedKey(translateKey(key), down);
        if (s_active->m_bus) {
            if (down) s_active->m_bus->post<KeyPressedEvent>(key, mods, false);
            else      s_active->m_bus->post<KeyReleasedEvent>(key, mods);
        }
    });

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow*, int button, int action, int mods) {
        if (!s_active) return;
        const bool down = (action == GLFW_PRESS);
        s_active->feedMouseButton(translateMouse(button), down);
        if (s_active->m_bus) s_active->m_bus->post<MouseButtonEvent>(button, action, mods);
    });

    glfwSetCursorPosCallback(m_window, [](GLFWwindow*, double x, double y) {
        if (!s_active) return;
        s_active->feedMousePosition(static_cast<f32>(x), static_cast<f32>(y));
        if (s_active->m_bus)
            s_active->m_bus->post<MouseMovedEvent>(static_cast<f32>(x), static_cast<f32>(y));
    });

    glfwSetScrollCallback(m_window, [](GLFWwindow*, double xoff, double yoff) {
        if (!s_active) return;
        s_active->feedScroll(static_cast<f32>(yoff));
        if (s_active->m_bus)
            s_active->m_bus->post<MouseScrollEvent>(static_cast<f32>(xoff), static_cast<f32>(yoff));
    });

    EMBER_LOG_INFO("InputManager initialized");
}

void InputManager::shutdown() {
    if (m_window) {
        glfwSetKeyCallback(m_window, nullptr);
        glfwSetMouseButtonCallback(m_window, nullptr);
        glfwSetCursorPosCallback(m_window, nullptr);
        glfwSetScrollCallback(m_window, nullptr);
        m_window = nullptr;
    }
    if (s_active == this) s_active = nullptr;
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------
void InputManager::update() {
    // Edges are computed against the snapshot taken at the previous update().
    for (usize i = 0; i < kKeyCount; ++i) {
        m_pressedKeys[i]  = m_curKeys[i] && !m_prevKeys[i];
        m_releasedKeys[i] = !m_curKeys[i] && m_prevKeys[i];
        m_prevKeys[i]     = m_curKeys[i];
    }
    for (usize i = 0; i < kMouseCount; ++i) {
        m_pressedMouse[i]  = m_curMouse[i] && !m_prevMouse[i];
        m_releasedMouse[i] = !m_curMouse[i] && m_prevMouse[i];
        m_prevMouse[i]     = m_curMouse[i];
    }

    m_mouseDelta   = m_mousePos - m_prevMousePos;
    m_prevMousePos = m_mousePos;

    m_scroll       = m_scrollAccum;
    m_scrollAccum  = 0.0f;

    pollGamepad();
}

// ---------------------------------------------------------------------------
// Raw state seam
// ---------------------------------------------------------------------------
void InputManager::feedKey(Key k, bool down) {
    if (k == Key::Unknown) return;
    const usize i = static_cast<usize>(k);
    if (i < kKeyCount) m_curKeys[i] = down;
}

void InputManager::feedMouseButton(Mouse m, bool down) {
    const usize i = static_cast<usize>(m);
    if (i < kMouseCount) m_curMouse[i] = down;
}

void InputManager::feedMousePosition(f32 x, f32 y) {
    m_mousePos = {x, y};
    if (m_firstMouse) { m_prevMousePos = m_mousePos; m_firstMouse = false; }
}

void InputManager::feedScroll(f32 yOffset) { m_scrollAccum += yOffset; }

// ---------------------------------------------------------------------------
// Polling
// ---------------------------------------------------------------------------
bool InputManager::isKeyDown(Key k) const {
    const usize i = static_cast<usize>(k);
    return i < kKeyCount && m_curKeys[i];
}
bool InputManager::isKeyPressed(Key k) const {
    const usize i = static_cast<usize>(k);
    return i < kKeyCount && m_pressedKeys[i];
}
bool InputManager::isKeyReleased(Key k) const {
    const usize i = static_cast<usize>(k);
    return i < kKeyCount && m_releasedKeys[i];
}
bool InputManager::isMouseButtonDown(Mouse m) const {
    const usize i = static_cast<usize>(m);
    return i < kMouseCount && m_curMouse[i];
}
bool InputManager::isMouseButtonPressed(Mouse m) const {
    const usize i = static_cast<usize>(m);
    return i < kMouseCount && m_pressedMouse[i];
}
bool InputManager::isMouseButtonReleased(Mouse m) const {
    const usize i = static_cast<usize>(m);
    return i < kMouseCount && m_releasedMouse[i];
}

// ---------------------------------------------------------------------------
// Gamepad
// ---------------------------------------------------------------------------
void InputManager::pollGamepad() {
    m_gamepadConnected = false;
    m_gamepadButtons.fill(false);
    m_gamepadAxes.fill(0.0f);

    GLFWgamepadstate state;
    if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1) &&
        glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
        m_gamepadConnected = true;
        for (usize i = 0; i < m_gamepadButtons.size() && i <= GLFW_GAMEPAD_BUTTON_LAST; ++i)
            m_gamepadButtons[i] = state.buttons[i] == GLFW_PRESS;
        for (usize i = 0; i < m_gamepadAxes.size() && i <= GLFW_GAMEPAD_AXIS_LAST; ++i) {
            const f32 v = state.axes[i];
            m_gamepadAxes[i] = (std::fabs(v) < m_gamepadDeadzone) ? 0.0f : v;
        }
    }
}

f32 InputManager::gamepadAxis(GamepadAxis a) const {
    const usize i = static_cast<usize>(a);
    return i < m_gamepadAxes.size() ? m_gamepadAxes[i] : 0.0f;
}
bool InputManager::isGamepadButtonDown(GamepadButton b) const {
    const usize i = static_cast<usize>(b);
    return i < m_gamepadButtons.size() && m_gamepadButtons[i];
}

// ---------------------------------------------------------------------------
// Action map
// ---------------------------------------------------------------------------
void InputManager::addAction(const InputAction& action) {
    const std::string ctx = m_contextStack.empty() ? std::string("game") : m_contextStack.back();
    m_contextActions[ctx][action.name()] = action;
}

ActionState InputManager::evaluate(const InputAction& a) const {
    ActionState st;
    for (const auto& b : a.bindings()) {
        switch (b.kind) {
            case InputAction::BindingKind::Key: {
                const Key k = static_cast<Key>(b.code);
                st.held     |= isKeyDown(k);
                st.pressed  |= isKeyPressed(k);
                st.released |= isKeyReleased(k);
                if (isKeyDown(k) && std::fabs(b.scale) > std::fabs(st.axisValue))
                    st.axisValue = b.scale;
                break;
            }
            case InputAction::BindingKind::MouseButton: {
                const Mouse m = static_cast<Mouse>(b.code);
                st.held     |= isMouseButtonDown(m);
                st.pressed  |= isMouseButtonPressed(m);
                st.released |= isMouseButtonReleased(m);
                break;
            }
            case InputAction::BindingKind::GamepadButton: {
                const GamepadButton gb = static_cast<GamepadButton>(b.code);
                st.held |= isGamepadButtonDown(gb);
                break;
            }
            case InputAction::BindingKind::GamepadAxis: {
                const f32 v = gamepadAxis(static_cast<GamepadAxis>(b.code)) * b.scale;
                if (std::fabs(v) > std::fabs(st.axisValue)) st.axisValue = v;
                if (std::fabs(v) > 0.0f) st.held = true;
                break;
            }
        }
    }
    return st;
}

ActionState InputManager::getAction(const std::string& name) const {
    if (m_contextStack.empty()) return {};
    const auto ctxIt = m_contextActions.find(m_contextStack.back());
    if (ctxIt == m_contextActions.end()) return {};
    const auto actIt = ctxIt->second.find(name);
    if (actIt == ctxIt->second.end()) return {};
    return evaluate(actIt->second);
}

void InputManager::loadDefaultActionMap() {
    m_contextActions.clear();
    auto& game = m_contextActions["game"];

    InputAction moveX("MoveX");
    moveX.bindKey(Key::D).bindKey(Key::Right)
         .bindKey(Key::A, -1.0f).bindKey(Key::Left, -1.0f)
         .bindAxis(GamepadAxis::LeftX, 1.0f);
    game["MoveX"] = moveX;

    InputAction moveY("MoveY");
    moveY.bindKey(Key::W).bindKey(Key::Up)
         .bindKey(Key::S, -1.0f).bindKey(Key::Down, -1.0f)
         .bindAxis(GamepadAxis::LeftY, -1.0f);
    game["MoveY"] = moveY;

    InputAction jump("Jump");
    jump.bindKey(Key::Space).bindGamepadButton(GamepadButton::A);
    game["Jump"] = jump;

    if (m_contextStack.empty()) m_contextStack.push_back("game");
}

bool InputManager::loadActionMap(const std::filesystem::path& path) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(path.string());
    } catch (const std::exception& e) {
        EMBER_LOG_WARN("InputManager: failed to load action map '{}': {}; using defaults",
                       path.string(), e.what());
        loadDefaultActionMap();
        return false;
    }

    const YAML::Node contexts = root["contexts"];
    if (!contexts || !contexts.IsMap()) {
        EMBER_LOG_WARN("InputManager: action map '{}' has no 'contexts' map; using defaults",
                       path.string());
        loadDefaultActionMap();
        return false;
    }

    m_contextActions.clear();
    for (auto ctxIt = contexts.begin(); ctxIt != contexts.end(); ++ctxIt) {
        const std::string ctxName = ctxIt->first.as<std::string>();
        const YAML::Node actions = ctxIt->second["actions"];
        if (!actions || !actions.IsMap()) continue;
        auto& dst = m_contextActions[ctxName];
        for (auto actIt = actions.begin(); actIt != actions.end(); ++actIt) {
            const std::string actName = actIt->first.as<std::string>();
            InputAction action(actName);
            for (const auto& b : actIt->second) {
                InputAction::Binding binding;
                binding.kind  = kindFromString(b["type"].as<std::string>("key"));
                binding.code  = static_cast<u16>(b["code"].as<int>(0));
                binding.scale = b["scale"].as<f32>(1.0f);
                action.bindings().push_back(binding);
            }
            dst[actName] = action;
        }
    }
    return true;
}

bool InputManager::saveActionMap(const std::filesystem::path& path) const {
    YAML::Node root;
    for (const auto& [ctxName, actions] : m_contextActions) {
        YAML::Node actionsNode;
        for (const auto& [actName, action] : actions) {
            YAML::Node bindingsNode;
            for (const auto& b : action.bindings()) {
                YAML::Node bn;
                bn["type"]  = kindToString(b.kind);
                bn["code"]  = static_cast<int>(b.code);
                bn["scale"] = b.scale;
                bindingsNode.push_back(bn);
            }
            actionsNode[actName] = bindingsNode;
        }
        root["contexts"][ctxName]["actions"] = actionsNode;
    }
    try {
        std::ofstream out(path);
        out << root;
    } catch (const std::exception& e) {
        EMBER_LOG_WARN("InputManager: failed to save action map '{}': {}", path.string(), e.what());
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Context stack
// ---------------------------------------------------------------------------
void InputManager::pushContext(const std::string& name) { m_contextStack.push_back(name); }

void InputManager::popContext() {
    if (!m_contextStack.empty()) m_contextStack.pop_back();
}

const std::string& InputManager::activeContext() const {
    static const std::string kNone;
    return m_contextStack.empty() ? kNone : m_contextStack.back();
}

// ===========================================================================
// Input:: facade (null-safe forwarding to the active manager)
// ===========================================================================
namespace Input {

bool isKeyDown(Key k)      { auto* m = InputManager::active(); return m && m->isKeyDown(k); }
bool isKeyPressed(Key k)   { auto* m = InputManager::active(); return m && m->isKeyPressed(k); }
bool isKeyReleased(Key k)  { auto* m = InputManager::active(); return m && m->isKeyReleased(k); }

bool isMouseButtonDown(Mouse b)     { auto* m = InputManager::active(); return m && m->isMouseButtonDown(b); }
bool isMouseButtonPressed(Mouse b)  { auto* m = InputManager::active(); return m && m->isMouseButtonPressed(b); }
bool isMouseButtonReleased(Mouse b) { auto* m = InputManager::active(); return m && m->isMouseButtonReleased(b); }

glm::vec2 getMousePosition() { auto* m = InputManager::active(); return m ? m->mousePosition() : glm::vec2(0.0f); }
glm::vec2 getMouseDelta()    { auto* m = InputManager::active(); return m ? m->mouseDelta()    : glm::vec2(0.0f); }
f32       getMouseScroll()   { auto* m = InputManager::active(); return m ? m->mouseScroll()   : 0.0f; }

f32  getGamepadAxis(GamepadAxis a)        { auto* m = InputManager::active(); return m ? m->gamepadAxis(a) : 0.0f; }
bool isGamepadButtonDown(GamepadButton b) { auto* m = InputManager::active(); return m && m->isGamepadButtonDown(b); }

ActionState getAction(const std::string& name) {
    auto* m = InputManager::active();
    return m ? m->getAction(name) : ActionState{};
}

} // namespace Input

} // namespace ember
