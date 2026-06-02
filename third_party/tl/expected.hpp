///
/// Minimal tl::expected-compatible implementation for Ember Engine.
/// API-compatible subset covering: expected<T,E>, unexpected<E>, has_value(),
/// value(), error(), operator*, operator->, and value_or().
///
/// License: CC0 / public domain — replace with the real tl::expected on any
/// platform that doesn't ship std::expected (GCC < 12, Clang < 16, MSVC < 19.36).
///
#pragma once
#include <variant>
#include <stdexcept>
#include <utility>
#include <type_traits>

namespace tl {

template<typename E>
class unexpected {
public:
    constexpr explicit unexpected(const E& e) : m_error(e) {}
    constexpr explicit unexpected(E&& e)      : m_error(std::move(e)) {}
    constexpr const E& value() const& noexcept { return m_error; }
    constexpr       E& value()      & noexcept { return m_error; }
    constexpr       E  value()     &&          { return std::move(m_error); }
private:
    E m_error;
};

template<typename E>
unexpected<std::decay_t<E>> make_unexpected(E&& e) {
    return unexpected<std::decay_t<E>>(std::forward<E>(e));
}

template<typename T, typename E>
class expected {
    static_assert(!std::is_same_v<T, void> || std::is_same_v<T, void>,
                  "void T is partially supported");
public:
    using value_type = T;
    using error_type = E;

    // Construct from value
    constexpr expected(const T& v) : m_data(std::in_place_index<0>, v) {}
    constexpr expected(T&& v)      : m_data(std::in_place_index<0>, std::move(v)) {}

    // Construct from unexpected
    constexpr expected(const unexpected<E>& u) : m_data(std::in_place_index<1>, u.value()) {}
    constexpr expected(unexpected<E>&& u)      : m_data(std::in_place_index<1>, std::move(u).value()) {}

    // Observers
    [[nodiscard]] constexpr bool has_value() const noexcept {
        return m_data.index() == 0;
    }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr const T&  value()  const& { _check(); return std::get<0>(m_data); }
    constexpr       T&  value()       & { _check(); return std::get<0>(m_data); }
    constexpr       T   value()      && { _check(); return std::move(std::get<0>(m_data)); }
    constexpr const T&  operator*() const& noexcept { return std::get<0>(m_data); }
    constexpr       T&  operator*()      & noexcept { return std::get<0>(m_data); }
    constexpr const T*  operator->() const noexcept { return &std::get<0>(m_data); }
    constexpr       T*  operator->()       noexcept { return &std::get<0>(m_data); }

    constexpr const E&  error()  const& noexcept { return std::get<1>(m_data); }
    constexpr       E&  error()       & noexcept { return std::get<1>(m_data); }

    template<typename U>
    constexpr T value_or(U&& def) const& {
        return has_value() ? **this : static_cast<T>(std::forward<U>(def));
    }

private:
    void _check() const {
        if (!has_value())
            throw std::runtime_error("tl::expected: accessing value of an error result");
    }
    std::variant<T, E> m_data;
};

// Partial specialisation for expected<void, E>
template<typename E>
class expected<void, E> {
public:
    using value_type = void;
    using error_type = E;

    constexpr expected() : m_error{}, m_hasValue(true) {}
    constexpr expected(const unexpected<E>& u) : m_error(u.value()), m_hasValue(false) {}
    constexpr expected(unexpected<E>&& u)      : m_error(std::move(u).value()), m_hasValue(false) {}

    [[nodiscard]] constexpr bool has_value() const noexcept { return m_hasValue; }
    constexpr explicit operator bool() const noexcept { return m_hasValue; }

    constexpr void value() const {
        if (!m_hasValue)
            throw std::runtime_error("tl::expected<void>: accessing value of an error result");
    }

    constexpr const E& error() const& noexcept { return m_error; }
    constexpr       E& error()      & noexcept { return m_error; }

private:
    E    m_error;
    bool m_hasValue;
};

} // namespace tl
