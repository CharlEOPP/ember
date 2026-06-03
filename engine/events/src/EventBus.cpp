#include "ember/events/EventBus.h"

#include <algorithm>

namespace ember {

// ---- SubscriptionToken ----

SubscriptionToken::~SubscriptionToken() { release(); }

void SubscriptionToken::release() {
    if (m_bus) {
        m_bus->unsubscribe(m_type, m_id);
        m_bus = nullptr;
    }
}

void SubscriptionToken::moveFrom(SubscriptionToken& other) {
    m_bus  = other.m_bus;
    m_type = other.m_type;
    m_id   = other.m_id;
    other.m_bus = nullptr; // moved-from token must not unsubscribe
}

SubscriptionToken& SubscriptionToken::operator=(SubscriptionToken&& other) noexcept {
    if (this != &other) {
        release();
        moveFrom(other);
    }
    return *this;
}

// ---- EventBus ----

void EventBus::dispatch() {
    // Snapshot channel pointers so a handler that posts a brand-new event *type*
    // during dispatch doesn't invalidate our iteration (it's handled next frame).
    // Node-based unordered_map keeps existing Channel addresses stable on insert.
    std::vector<Channel*> channels;
    channels.reserve(m_channels.size());
    for (auto& [type, ch] : m_channels) {
        (void)type;
        channels.push_back(&ch);
    }

    for (Channel* ch : channels) {
        // Swap the queue out first so events posted *during* dispatch are
        // deferred to the next dispatch rather than processed this frame.
        std::deque<std::any> pending;
        std::swap(pending, ch->queue);
        for (const std::any& ev : pending)
            for (const Sub& sub : ch->subs)
                sub.call(ev);
    }
}

void EventBus::unsubscribe(std::type_index type, u64 id) {
    auto it = m_channels.find(type);
    if (it == m_channels.end()) return;
    auto& subs = it->second.subs;
    std::erase_if(subs, [id](const Sub& s) { return s.id == id; });
}

} // namespace ember
