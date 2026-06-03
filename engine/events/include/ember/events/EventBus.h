#pragma once
//
// EventBus — deferred, buffered, per-frame event queue.
//
// Threading contract: post(), subscribe(), reserve(), and dispatch() are
// single-threaded and MUST be called from the main thread only.
//
#include "ember/core/Types.h"
#include "ember/core/Log.h"

#include <any>
#include <deque>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ember {

class EventBus;

// RAII subscription handle. Unsubscribes its handler when destroyed.
// Move-only: a moved-from token does nothing on destruction.
class SubscriptionToken {
public:
    SubscriptionToken() = default;
    SubscriptionToken(EventBus* bus, std::type_index type, u64 id)
        : m_bus(bus), m_type(type), m_id(id) {}
    ~SubscriptionToken();

    SubscriptionToken(const SubscriptionToken&)            = delete;
    SubscriptionToken& operator=(const SubscriptionToken&) = delete;
    SubscriptionToken(SubscriptionToken&& other) noexcept { moveFrom(other); }
    SubscriptionToken& operator=(SubscriptionToken&& other) noexcept;

    [[nodiscard]] bool active() const { return m_bus != nullptr; }

private:
    void moveFrom(SubscriptionToken& other);
    void release();

    EventBus*       m_bus  = nullptr;                       // null => empty / moved-from
    std::type_index m_type = std::type_index(typeid(void));
    u64             m_id   = 0;
};

class EventBus {
public:
    EventBus() = default;
    EventBus(const EventBus&)            = delete;
    EventBus& operator=(const EventBus&) = delete;

    // Enqueue an event for later dispatch. Never invokes subscribers immediately.
    template<typename T, typename... Args>
    void post(Args&&... args) {
        Channel& ch = channel<T>();
        if (ch.queue.size() >= ch.capacity) {
            EMBER_LOG_WARN("EventBus: ring buffer for '{}' full (cap {}), dropping event",
                           typeid(T).name(), ch.capacity);
            return;
        }
        ch.queue.emplace_back(std::any(T{std::forward<Args>(args)...}));
    }

    // Register a handler; returns an RAII token that unsubscribes on destruction.
    template<typename T>
    [[nodiscard]] SubscriptionToken subscribe(std::function<void(const T&)> handler) {
        Channel& ch = channel<T>();
        const u64 id = ++ch.nextId;
        ch.subs.push_back(Sub{ id, [h = std::move(handler)](const std::any& e) {
            h(std::any_cast<const T&>(e));
        }});
        return SubscriptionToken{ this, std::type_index(typeid(T)), id };
    }

    // Override the ring-buffer capacity for a specific event type (default 256).
    template<typename T>
    void reserve(usize capacity) { channel<T>().capacity = capacity; }

    // Drain every queued event to its subscribers (FIFO), then clear the queues.
    // Call once per frame from the main loop.
    void dispatch();

private:
    friend class SubscriptionToken;

    struct Sub {
        u64                                   id = 0;
        std::function<void(const std::any&)>  call;
    };

    struct Channel {
        usize                capacity = 256;
        std::deque<std::any> queue;
        std::vector<Sub>     subs;
        u64                  nextId = 0;
    };

    template<typename T>
    Channel& channel() { return m_channels[std::type_index(typeid(T))]; }

    void unsubscribe(std::type_index type, u64 id);

    std::unordered_map<std::type_index, Channel> m_channels;
};

} // namespace ember
