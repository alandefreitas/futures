//
// Created by alandefreitas on 10/15/21.
//

#ifndef FUTURES_DETAIL_TRAITS_FUTURE_VALUE_HPP
#define FUTURES_DETAIL_TRAITS_FUTURE_VALUE_HPP

#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    namespace detail {
        // Check if a type implements the get function
        template <class T, typename = void>
        struct has_get : std::false_type {};

        template <class T>
        struct has_get<T, void_t<decltype(std::declval<T>().get())>>
            : std::true_type {};
    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_TRAITS_FUTURE_VALUE_HPP
