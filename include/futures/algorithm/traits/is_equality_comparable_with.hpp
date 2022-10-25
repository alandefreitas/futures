//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_WITH_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_WITH_HPP

/**
 *  @file algorithm/traits/is_equality_comparable_with.hpp
 *  @brief `is_equality_comparable_with` trait
 *
 *  This file defines the `is_equality_comparable_with` trait.
 */

#include <futures/algorithm/traits/is_equality_comparable.hpp>
#include <futures/algorithm/traits/is_weakly_equality_comparable.hpp>
#include <type_traits>

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
     * @see https://en.cppreference.com/w/cpp/concepts/equality_comparable_with
     */
    template <class T, class U>
    using is_equality_comparable_with = std::conjunction<
        is_equality_comparable<T>,
        is_equality_comparable<U>,
        is_weakly_equality_comparable<T, U>>;

    /// @copydoc is_equality_comparable_with
    template <class T, class U>
    constexpr bool is_equality_comparable_with_v
        = is_equality_comparable_with<T, U>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_WITH_HPP
