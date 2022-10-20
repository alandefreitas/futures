//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_DETAIL_IS_SUBTRACTABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_DETAIL_IS_SUBTRACTABLE_HPP

#include <type_traits>

namespace futures::detail {
    template <class T, class = void>
    struct is_subtractable : std::false_type {};

    template <class T>
    struct is_subtractable<
        T,
        std::void_t<
            // clang-format off
            decltype(std::declval< std::remove_reference_t<T> const &>() - std::declval< std::remove_reference_t<T> const &>())
            // clang-format on
            >>
        : std::is_integral<
              decltype(std::declval<std::remove_reference_t<T> const &>() - std::declval<std::remove_reference_t<T> const &>())> {
    };

    template <class T>
    constexpr bool is_subtractable_v = is_subtractable<T>::value;
} // namespace futures::detail

#endif // FUTURES_ALGORITHM_TRAITS_DETAIL_IS_SUBTRACTABLE_HPP
