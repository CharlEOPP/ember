#pragma once
#include "ember/core/Types.h"

// Ember input enums. Deliberately GLFW-free: GLFW keycodes are translated to
// these values inside InputManager.cpp at the platform boundary, so no GLFW
// type ever leaks into a public header (epic.md "Notes & Decisions").
//
// Numeric values are explicit and stable so action maps can be serialized by
// integer value without breaking when the enum is reordered.

namespace ember {

enum class Key : u16 {
    Unknown = 0,

    // Printable
    Space = 32,
    Apostrophe = 39,   // '
    Comma = 44,        // ,
    Minus = 45,        // -
    Period = 46,       // .
    Slash = 47,        // /
    D0 = 48, D1, D2, D3, D4, D5, D6, D7, D8, D9,
    Semicolon = 59,    // ;
    Equal = 61,        // =
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = 91,  // [
    Backslash = 92,    // backslash
    RightBracket = 93, // ]
    GraveAccent = 96,  // `

    // Function / control
    Escape = 256, Enter, Tab, Backspace, Insert, Delete,
    Right, Left, Down, Up,
    PageUp, PageDown, Home, End,
    CapsLock = 280, ScrollLock, NumLock, PrintScreen, Pause,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Keypad
    KP0 = 320, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
    KPDecimal, KPDivide, KPMultiply, KPSubtract, KPAdd, KPEnter, KPEqual,

    // Modifiers
    LeftShift = 340, LeftControl, LeftAlt, LeftSuper,
    RightShift, RightControl, RightAlt, RightSuper, Menu,

    Count = 350 // upper bound for state arrays
};

enum class Mouse : u8 {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    Count = 5
};

// Standard gamepad mapping (matches GLFW's gamepad mapping db ordering).
enum class GamepadButton : u8 {
    A = 0, B, X, Y,
    LeftBumper, RightBumper,
    Back, Start, Guide,
    LeftThumb, RightThumb,
    DPadUp, DPadRight, DPadDown, DPadLeft,
    Count
};

enum class GamepadAxis : u8 {
    LeftX = 0, LeftY,
    RightX, RightY,
    LeftTrigger, RightTrigger,
    Count
};

constexpr usize kKeyCount   = static_cast<usize>(Key::Count);
constexpr usize kMouseCount = static_cast<usize>(Mouse::Count);

} // namespace ember
