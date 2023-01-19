//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_TOTALLY_ORDERED_WITH_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_TOTALLY_ORDERED_WITH_HPP

/**
 *  @file algorithm/traits/is_totally_ordered_with.hpp
 *  @brief `is_totally_ordered_with` trait
 *
 *  This file defines the `is_totally_ordered_with` trait.
 */

#include <futures/algorithm/traits/is_equality_comparable.hpp>
#include <futures/algorithm/traits/is_equality_comparable_with.hpp>
#include <futures/algorithm/traits/is_totally_ordered.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/algorithm/traits/detail/is_partially_ordered_with.hpp>
#include <futures/algorithm/traits/detail/is_weakly_equality_comparable_with.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::totally_ordered_with` concept
    /**
     * @see
     * [`std::totally_ordered`](https://en.cppreference.com/w/cpp/concepts/totally_ordered)
     */
#if defined(FUTURES_DOXYGEN) || defined(__cpp_lib_three_way_comparison)
    template <class T, class U>
    using is_totally_ordered_with = std::bool_constant<
        std::totally_ordered_with<T, U>>;
#else
    template <class T, class U>
    using is_totally_ordered_with = detail::conjunction<
        is_totally_ordered<T>,
        is_totally_ordered<U>,
        is_equality_comparable_with<T, U>,
        detail::is_partially_ordered_with<T, U>>;
#endif

    /// @copydoc is_totally_ordered_with
    template <class T, class U>
    constexpr bool is_totally_ordered_with_v = is_totally_ordered_with<T, U>::
        value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_TOTALLY_ORDERED_WITH_HPP
