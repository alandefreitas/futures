//
// Created by alandefreitas on 10/15/21.
//

#ifndef FUTURES_DETAIL_IS_READY_HPP
#define FUTURES_DETAIL_IS_READY_HPP

#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    namespace detail {
        // Check if a type implements the is_ready function and it returns bool
        // This is what we use to identify the return type of a future type
        // candidate However, this doesn't mean the type is a future in the
        // terms of the is_future concept
        template <class T, typename = void>
        struct has_is_ready : std::false_type {};

        template <class T>
        struct has_is_ready<T, void_t<decltype(std::declval<T>().is_ready())>>
            : std::is_same<bool, decltype(std::declval<T>().is_ready())> {};

        template <class T>
        constexpr bool has_is_ready_v = has_is_ready<T>::value;

    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_IS_READY_HPP
