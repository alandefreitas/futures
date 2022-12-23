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
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/algorithm/traits/detail/is_partially_ordered_with.hpp>
#include <type_traits>

#ifdef __cpp_lib_three_way_comparison
#    include <compare>
#endif

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
#if defined(FUTURES_DOXYGEN) || defined(__cpp_lib_three_way_comparison)
    template <class T>
    using is_totally_ordered = std::bool_constant<std::totally_ordered<T>>;
#else
    template <class T>
    using is_totally_ordered = detail::conjunction<
        is_equality_comparable<T>,
        detail::is_partially_ordered_with<T, T>>;
#endif

    /// @copydoc is_totally_ordered
    template <class T>
    constexpr bool is_totally_ordered_v = is_totally_ordered<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_TOTALLY_ORDERED_HPP
