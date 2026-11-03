#pragma once

namespace std {
    template<typename T, T v>
    struct integral_constant { static constexpr T value = v; typedef T type; };

    using true_type = integral_constant<bool, true>;
    using false_type = integral_constant<bool, false>;

    template<typename T> struct is_lvalue_reference : false_type {};
    template<typename T> struct is_lvalue_reference<T&> : true_type {};

    template<typename T> struct is_rvalue_reference : false_type {};
    template<typename T> struct is_rvalue_reference<T&&> : true_type {};

    template<typename T> struct remove_reference { typedef T type; };
    template<typename T> struct remove_reference<T&>  { typedef T type; };
    template<typename T> struct remove_reference<T&&> { typedef T type; };

    template<typename T>
    T&& forward(typename remove_reference<T>::type& t) noexcept {
        return static_cast<T&&>(t);
    }

    template<typename T>
    T&& forward(typename remove_reference<T>::type&& t) noexcept {
        static_assert(!is_lvalue_reference<T>::value, "bad forward");
        return static_cast<T&&>(t);
    }
}
