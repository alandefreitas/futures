//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_THREE_WAY_COMPARABLE_WITH_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_THREE_WAY_COMPARABLE_WITH_HPP

/**
 *  @file algorithm/traits/is_three_way_comparable_with.hpp
 *  @brief `is_three_way_comparable_with` trait
 *
 *  This file defines the `is_three_way_comparable_with` trait.
 */

#include <futures/config.hpp>
#include <futures/algorithm/traits/is_three_way_comparable.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/algorithm/traits/detail/is_partially_ordered_with.hpp>
#include <futures/algorithm/traits/detail/is_weakly_equality_comparable_with.hpp>
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

    /// @brief A type trait equivalent to the `std::equality_comparable_with`
    /// concept
    /**
     * @see
     * https://en.cppreference.com/w/cpp/utility/compare/three_way_comparable
     */
#if defined(FUTURES_DOXYGEN) || defined(__cpp_lib_three_way_comparison)
    template <class T, class U, class Cat = std::partial_ordering>
    using is_three_way_comparable_with = std::bool_constant<
        std::three_way_comparable_with<T, U, Cat>>;
#else
    template <class T, class U, class Cat = partial_ordering>
    using is_three_way_comparable_with = detail::conjunction<
        is_three_way_comparable<T>,
        is_three_way_comparable<U>,
        detail::is_weakly_equality_comparable_with<T, U>,
        detail::is_partially_ordered_with<T, U>>;
#endif

    /// @copydoc is_three_way_comparable_with
    template <class T, class U>
    constexpr bool is_three_way_comparable_with_v
        = is_three_way_comparable_with<T, U>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_THREE_WAY_COMPARABLE_WITH_HPP
