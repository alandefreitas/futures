//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_DETAIL_IS_EXECUTOR_HPP
#define FUTURES_EXECUTOR_DETAIL_IS_EXECUTOR_HPP

#include <futures/config.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/deps/asio/is_executor.hpp>

namespace futures {
    namespace detail {
        struct invocable_archetype {
            void
            operator()() const {}
        };

        // Check if a type implements the get_executor() function
        template <class T, typename = void>
        struct has_get_executor : std::false_type {};

        template <class T>
        struct has_get_executor<
            T,
            void_t<decltype(std::declval<T>().get_executor())>>
            : std::true_type {};

        // Check if a type implements the get function
        template <class T, typename = void>
        struct has_execute : std::false_type {};

        template <class T>
        struct has_execute<
            T,
            void_t<decltype(std::declval<T>().execute(
                std::declval<invocable_archetype>()))>> : std::true_type {};

        // Check if a type implements the get function but not the get_execute
        // This is a workaround for GCC7, which fails to return false for
        // has_execute when the class has a private execute function
        template <class T, class = void>
        struct is_light_executor_impl : std::false_type {};

        template <class T>
        struct is_light_executor_impl<
            T,
            std::enable_if_t<
                // clang-format off
                !detail::has_get_executor<T>::value
                // clang-format on
                >> : has_execute<T> {};

        // Check if a type implements the get function
        template <class T>
        using is_executor_impl = conjunction<
            is_light_executor_impl<T>,
            std::is_copy_constructible<T>>;
    } // namespace detail
} // namespace futures

#endif // FUTURES_EXECUTOR_DETAIL_IS_EXECUTOR_HPP
