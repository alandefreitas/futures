//
// Created by alandefreitas on 10/15/21.
//

#ifndef FUTURES_DETAIL_TRAITS_STD_TYPE_TRAITS_HPP
#define FUTURES_DETAIL_TRAITS_STD_TYPE_TRAITS_HPP

#include <futures/config.hpp>
#include <futures/detail/deps/boost/mp11/function.hpp>
#include <system_error>
#include <type_traits>

namespace futures {
    namespace detail {
        template <class T>
        FUTURES_INLINE_VAR constexpr bool is_lvalue_reference_v = std::
            is_lvalue_reference<T>::value;

        template <class T>
        FUTURES_INLINE_VAR constexpr bool is_object_v = std::is_object<T>::value;

        template <class T>
        FUTURES_INLINE_VAR constexpr bool is_function_v = std::is_function<
            T>::value;

        template <bool B, class T1, class T2>
        using conditional_t = typename std::conditional<B, T1, T2>::type;

        template <class... Bn>
        using conjunction = mp_and<Bn...>;

        template <class... B>
        FUTURES_INLINE_VAR constexpr bool conjunction_v = conjunction<
            B...>::value;

        template <class E>
        FUTURES_INLINE_VAR constexpr bool is_error_code_enum_v = std::
            is_error_code_enum<E>::value;

        template <class E>
        FUTURES_INLINE_VAR constexpr bool is_enum_v = std::is_enum<E>::value;

        template <class T, class U>
        FUTURES_INLINE_VAR constexpr bool is_same_v = std::is_same<T, U>::value;

        template <class T>
        struct is_void
            : std::is_same<void, typename std::remove_cv<T>::type> {};

        template <class T>
        FUTURES_INLINE_VAR constexpr bool is_void_v = is_void<T>::value;

        template <class T>
        FUTURES_INLINE_VAR constexpr bool is_reference_v = std::is_reference<
            T>::value;

        template <class T>
        FUTURES_INLINE_VAR constexpr bool is_trivial_v = std::is_trivial<
            T>::value;

        template <class T>
        FUTURES_INLINE_VAR constexpr bool is_integral_v = std::is_integral<
            T>::value;

        template <class T>
        FUTURES_INLINE_VAR constexpr bool is_copy_constructible_v = std::
            is_copy_constructible<T>::value;

        template <class T>
        FUTURES_INLINE_VAR constexpr bool is_move_constructible_v = std::
            is_move_constructible<T>::value;

        template <class T>
        FUTURES_INLINE_VAR constexpr bool is_nothrow_move_constructible_v
            = std::is_nothrow_move_constructible<T>::value;

        template <class T, class U>
        FUTURES_INLINE_VAR constexpr bool is_convertible_v = std::
            is_convertible<T, U>::value;

        template <class T, class... Args>
        FUTURES_INLINE_VAR constexpr bool is_constructible_v = std::
            is_constructible<T, Args...>::value;

        template <class T, class... Args>
        FUTURES_INLINE_VAR constexpr bool is_nothrow_constructible_v = std::
            is_nothrow_constructible<T, Args...>::value;

        template <class T, class U>
        FUTURES_INLINE_VAR constexpr bool is_base_of_v = std::is_base_of<T, U>::
            value;

        template <class...>
        struct disjunction : std::false_type {};
        template <class B1>
        struct disjunction<B1> : B1 {};
        template <class B1, class... Bn>
        struct disjunction<B1, Bn...>
            : conditional_t<bool(B1::value), B1, disjunction<Bn...>> {};

        template <class... B>
        FUTURES_INLINE_VAR constexpr bool disjunction_v = disjunction<
            B...>::value;

        template <bool B>
        using bool_constant = std::integral_constant<bool, B>;

        template <class T>
        using decay_t = typename std::decay<T>::type;

        template <typename... Ts>
        struct make_void {
            typedef void type;
        };

        template <typename... Ts>
        using void_t = typename make_void<Ts...>::type;

        struct in_place_t {
            explicit in_place_t() = default;
        };
        FUTURES_INLINE_VAR constexpr in_place_t in_place{};

        template <class T>
        struct in_place_type_t {
            explicit in_place_type_t() = default;
        };
        template <class T>
        FUTURES_INLINE_VAR constexpr in_place_type_t<T> in_place_type{};

        template <class T>
        constexpr std::add_const_t<T>&
        as_const(T& t) noexcept {
            return t;
        }

    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_TRAITS_STD_TYPE_TRAITS_HPP
