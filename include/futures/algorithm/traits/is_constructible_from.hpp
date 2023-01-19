//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_CONSTRUCTIBLE_FROM_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_CONSTRUCTIBLE_FROM_HPP

/**
 *  @file algorithm/traits/is_constructible_from.hpp
 *  @brief `is_constructible_from` trait
 *
 *  This file defines the `is_constructible_from` trait.
 */

#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::constructible_from` concept
    /**
     * @see
     * [`std::constructible_from`](https://en.cppreference.com/w/cpp/concepts/constructible_from)
     */
    template <class T, class... Args>
    using is_constructible_from = detail::
        conjunction<std::is_destructible<T>, std::is_constructible<T, Args...>>;

    /// @copydoc is_constructible_from
    template <class T, class... Args>
    constexpr bool is_constructible_from_v = is_constructible_from<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_CONSTRUCTIBLE_FROM_HPP
