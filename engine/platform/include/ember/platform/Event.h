#pragma once
#include "ember/core/Types.h"
#include <string>

namespace ember {

enum class EventType {
    None = 0,
    WindowClose,
    WindowResize,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled
};

class Event {
public:
    virtual ~Event() = default;
    virtual EventType   getType()  const = 0;
    virtual std::string getName()  const = 0;
    bool handled = false;
};

// ---- Concrete event types (platform-generated) ----

struct WindowCloseEvent : Event {
    EventType   getType()  const override { return EventType::WindowClose; }
    std::string getName()  const override { return "WindowCloseEvent"; }
};

struct WindowResizeEvent : Event {
    u32 width, height;
    WindowResizeEvent(u32 w, u32 h) : width(w), height(h) {}
    EventType   getType()  const override { return EventType::WindowResize; }
    std::string getName()  const override { return "WindowResizeEvent"; }
};

} // namespace ember
