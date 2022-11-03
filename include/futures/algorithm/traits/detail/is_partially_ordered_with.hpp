//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_DETAIL_IS_PARTIALLY_ORDERED_WITH_HPP
#define FUTURES_ALGORITHM_TRAITS_DETAIL_IS_PARTIALLY_ORDERED_WITH_HPP

#include <type_traits>

namespace futures::detail {
    template <class T, class U, class = void>
    struct is_partially_ordered_with : std::false_type {};

    template <class T, class U>
    struct is_partially_ordered_with<
        T,
        U,
        std::void_t<
            // clang-format off
            decltype(std::declval< std::remove_reference_t<T> const &>() < std::declval< std::remove_reference_t<U> const &>()),
            decltype(std::declval< std::remove_reference_t<T> const &>() > std::declval< std::remove_reference_t<U> const &>()),
            decltype(std::declval< std::remove_reference_t<T> const &>() <= std::declval< std::remove_reference_t<U> const &>()),
            decltype(std::declval< std::remove_reference_t<T> const &>() >= std::declval< std::remove_reference_t<U> const &>()),
            decltype(std::declval< std::remove_reference_t<U> const &>() < std::declval< std::remove_reference_t<T> const &>()),
            decltype(std::declval< std::remove_reference_t<U> const &>() > std::declval< std::remove_reference_t<T> const &>()),
            decltype(std::declval< std::remove_reference_t<U> const &>() <= std::declval< std::remove_reference_t<T> const &>()),
            decltype(std::declval< std::remove_reference_t<U> const &>() >= std::declval< std::remove_reference_t<T> const &>())
            // clang-format on
            >> : std::true_type {};
} // namespace futures::detail

#endif // FUTURES_ALGORITHM_TRAITS_DETAIL_IS_PARTIALLY_ORDERED_WITH_HPP
