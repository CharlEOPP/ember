#pragma once
//
// Signal<T> — immediate, synchronous multicast callback.
//
// Note: component-lifecycle hooks use EnTT's *native* signals via
// World::on_construct/on_destroy (see engine/ecs/World.h). This Signal<T> is a
// lightweight, lambda-friendly immediate dispatcher for general engine use,
// where stateful capturing lambdas (which EnTT sinks do not accept) are wanted.
//
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace ember {

template<typename T>
class Signal {
public:
    template<typename F>
    void connect(F&& fn) { m_handlers.emplace_back(std::forward<F>(fn)); }

    void fire(const T& event) const {
        for (const auto& h : m_handlers) h(event);
    }

    void        clear()       { m_handlers.clear(); }
    std::size_t size()  const { return m_handlers.size(); }

private:
    std::vector<std::function<void(const T&)>> m_handlers;
};

} // namespace ember
