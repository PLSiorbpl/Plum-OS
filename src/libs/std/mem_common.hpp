#pragma once

namespace std {
    template <typename T>
    constexpr T&& move(T& t) noexcept {
        return static_cast<T&&>(t);
    }
}