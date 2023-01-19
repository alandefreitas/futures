//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_UTILITY_INVOKE_HPP
#define FUTURES_DETAIL_UTILITY_INVOKE_HPP

#include <futures/config.hpp>
#include <futures/detail/traits/is_reference_wrapper.hpp>
#include <futures/detail/utility/impl/invoke.hpp>
#include <utility>
#include <type_traits>

/*
 * A C++14 version of std::invoke and related traits
 */

namespace futures {
    namespace detail {
        // std::invoke_result
        template <class F, class... ArgTypes>
        struct invoke_result : public invoke_result_impl<F, ArgTypes...> {};

        // std::invoke_result_t
        template <class Function, class... Args>
        using invoke_result_t = typename invoke_result<Function, Args...>::type;

        // is_invocable
        template <typename Function, typename... Args>
        struct is_invocable
            : is_invocable_impl<invoke_result<Function, Args...>, void>::type {
        };

        // is_invocable_v
        template <class Function, class... Args>
        FUTURES_INLINE_VAR constexpr bool is_invocable_v
            = is_invocable<Function, Args...>::value;

        // is_invocable_r
        template <class R, class F, class... Args>
        struct is_invocable_r
            : is_invocable_impl<invoke_result<F, Args...>, R>::type {};

        // is_invocable_r_v
        template <class R, class F, class... Args>
        FUTURES_INLINE_VAR constexpr bool is_invocable_r_v
            = is_invocable_r<R, F, Args...>::value;

        // is_nothrow_invocable_r
        template <typename R, typename F, typename... Args>
        struct is_nothrow_invocable_r
            : conjunction<
                  is_nothrow_invocable_r_impl<invoke_result<F, Args...>, R>,
                  call_is_nothrow<F, Args...>>::type {};

        // is_nothrow_invocable_r_v
        template <class R, class F, class... Args>
        FUTURES_INLINE_VAR constexpr bool is_nothrow_invocable_r_v
            = is_nothrow_invocable_r<R, F, Args...>::value;

        // std::is_nothrow_invocable
        template <class Fn, class... Args>
        struct is_nothrow_invocable
            : public is_nothrow_invocable_impl<Fn, Args...> {};

        // std::is_nothrow_invocable_v
        template <class Function, class... Args>
        FUTURES_INLINE_VAR constexpr bool is_nothrow_invocable_v
            = is_nothrow_invocable<Function, Args...>::value;

        // std::invoke
        template <
            class F,
            class... Args,
            typename std::enable_if<is_invocable_v<F, Args...>, int>::type = 0>
        constexpr invoke_result_t<F, Args...>
        invoke(F&& f, Args&&... args) noexcept(
            is_nothrow_invocable_v<F, Args...>) {
            // f might be a regular function or a member pointer in a class
            return invoke_impl(std::forward<F>(f), std::forward<Args>(args)...);
        }

        // std::invoke_r
        template <
            class R,
            class F,
            class... Args,
            typename std::enable_if<is_invocable_r_v<R, F, Args...>, int>::type
            = 0>
        constexpr R
        invoke_r(F&& f, Args&&... args) noexcept(
            is_nothrow_invocable_r_v<R, F, Args...>) {
            // f might be a regular function or a member pointer in a class
            return invoke_r_impl<R>(
                is_void<R>{},
                std::forward<F>(f),
                std::forward<Args>(args)...);
        }
    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_UTILITY_INVOKE_HPP
