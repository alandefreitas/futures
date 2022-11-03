//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_THREE_WAY_COMPARABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_THREE_WAY_COMPARABLE_HPP

/**
 *  @file algorithm/traits/is_three_way_comparable.hpp
 *  @brief `is_three_way_comparable` trait
 *
 *  This file defines the `is_three_way_comparable` trait.
 */

#include <futures/config.hpp>
#include <futures/algorithm/compare/partial_ordering.hpp>
#include <futures/algorithm/compare/strong_ordering.hpp>
#include <futures/algorithm/compare/weak_ordering.hpp>
#include <futures/algorithm/traits/common_comparison_category.hpp>
#include <futures/algorithm/traits/is_equality_comparable.hpp>
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

    /// @brief A type trait equivalent to the `std::equality_comparable` concept
    /**
     * @see
     * https://en.cppreference.com/w/cpp/utility/compare/three_way_comparable
     */
#if defined(FUTURES_DOXYGEN) || defined(__cpp_lib_three_way_comparison)
    template <class T, class Cat = std::partial_ordering>
    using is_three_way_comparable = std::bool_constant<
        std::three_way_comparable<T, Cat>>;
#else
    template <class T, class Cat = partial_ordering>
    using is_three_way_comparable = std::conjunction<
        detail::is_weakly_equality_comparable_with<T, T>,
        detail::is_partially_ordered_with<T, T>>;
#endif

    /// @copydoc is_three_way_comparable
    template <class T, class Cat = partial_ordering>
    constexpr bool is_three_way_comparable_v = is_three_way_comparable<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_THREE_WAY_COMPARABLE_HPP
