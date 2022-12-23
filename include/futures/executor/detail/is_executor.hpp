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

        // Check if a type implements the get function
        template <class T, typename = void>
        struct has_execute : std::false_type {};

        template <class T>
        struct has_execute<
            T,
            void_t<decltype(std::declval<T>().execute(
                std::declval<invocable_archetype>()))>> : std::true_type {};
    } // namespace detail
} // namespace futures

#endif // FUTURES_EXECUTOR_DETAIL_IS_EXECUTOR_HPP
