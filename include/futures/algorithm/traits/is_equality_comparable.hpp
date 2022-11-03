//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_HPP

/**
 *  @file algorithm/traits/is_equality_comparable.hpp
 *  @brief `is_equality_comparable` trait
 *
 *  This file defines the `is_equality_comparable` trait.
 */

#include <futures/config.hpp>
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


    /// @brief A type trait equivalent to the `std::equality_comparable` concept
    /**
     * @see https://en.cppreference.com/w/cpp/concepts/equality_comparable
     */
#if defined(FUTURES_DOXYGEN) || defined(__cpp_lib_three_way_comparison)
    template <class T>
    using is_equality_comparable = std::bool_constant<
        std::equality_comparable<T>>;
#else
    template <class T>
    using is_equality_comparable = detail::
        is_weakly_equality_comparable_with<T, T>;
#endif

    /// @copydoc is_equality_comparable
    template <class T>
    constexpr bool is_equality_comparable_v = is_equality_comparable<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_HPP
