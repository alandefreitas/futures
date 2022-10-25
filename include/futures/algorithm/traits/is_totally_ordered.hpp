//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_TOTALLY_ORDERED_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_TOTALLY_ORDERED_HPP

/**
 *  @file algorithm/traits/is_totally_ordered.hpp
 *  @brief `is_totally_ordered` trait
 *
 *  This file defines the `is_totally_ordered` trait.
 */

#include <futures/algorithm/traits/is_equality_comparable.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /// @brief A type trait equivalent to the `std::totally_ordered` concept
    /**
     * @see https://en.cppreference.com/w/cpp/concepts/totally_ordered
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_totally_ordered = __see_below__;
#else
    template <class T, class = void>
    struct is_totally_ordered : std::false_type {};

    template <class T>
    struct is_totally_ordered<
        T,
        std::void_t<
            // clang-format off
            decltype(std::declval< std::remove_reference_t<T> const &>() < std::declval< std::remove_reference_t<T> const &>()),
            decltype(std::declval< std::remove_reference_t<T> const &>() > std::declval< std::remove_reference_t<T> const &>()),
            decltype(std::declval< std::remove_reference_t<T> const &>() <= std::declval< std::remove_reference_t<T> const &>()),
            decltype(std::declval< std::remove_reference_t<T> const &>() >= std::declval< std::remove_reference_t<T> const &>())
            // clang-format on
            >> : is_equality_comparable<T> {};
#endif

    /// @copydoc is_totally_ordered
    template <class T>
    constexpr bool is_totally_ordered_v = is_totally_ordered<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_TOTALLY_ORDERED_HPP
