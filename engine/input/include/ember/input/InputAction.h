#pragma once
#include "ember/input/Keys.h"
#include <string>
#include <vector>

namespace ember {

// State of a named action for the current frame.
struct ActionState {
    bool pressed   = false; // any binding went down this frame
    bool held      = false; // any binding is down
    bool released  = false; // any binding went up this frame
    f32  axisValue = 0.0f;  // largest-magnitude bound axis (-1..1), or 1/0 for buttons
};

// A named action bound to one or more physical inputs. An action fires if ANY
// of its bindings fire (logical OR), which is what lets "Jump" map to both
// Space and the gamepad A button simultaneously.
class InputAction {
public:
    enum class BindingKind : u8 { Key, MouseButton, GamepadButton, GamepadAxis };

    struct Binding {
        BindingKind kind = BindingKind::Key;
        u16         code = 0;     // Key / Mouse / GamepadButton / GamepadAxis value
        f32         scale = 1.0f; // axis scaling (and sign)
    };

    InputAction() = default;
    explicit InputAction(std::string name) : m_name(std::move(name)) {}

    InputAction& bindKey(Key k) {
        m_bindings.push_back({BindingKind::Key, static_cast<u16>(k), 1.0f});
        return *this;
    }
    // Bind a key contributing `scale` to the action's axis (e.g. -1 for "left"/"down").
    InputAction& bindKey(Key k, f32 scale) {
        m_bindings.push_back({BindingKind::Key, static_cast<u16>(k), scale});
        return *this;
    }
    InputAction& bindMouseButton(Mouse m) {
        m_bindings.push_back({BindingKind::MouseButton, static_cast<u16>(m), 1.0f});
        return *this;
    }
    InputAction& bindGamepadButton(GamepadButton b) {
        m_bindings.push_back({BindingKind::GamepadButton, static_cast<u16>(b), 1.0f});
        return *this;
    }
    InputAction& bindAxis(GamepadAxis a, f32 scale = 1.0f) {
        m_bindings.push_back({BindingKind::GamepadAxis, static_cast<u16>(a), scale});
        return *this;
    }

    const std::string&          name()     const { return m_name; }
    const std::vector<Binding>&  bindings() const { return m_bindings; }
    std::vector<Binding>&        bindings()       { return m_bindings; }

private:
    std::string          m_name;
    std::vector<Binding> m_bindings;
};

} // namespace ember
