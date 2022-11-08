//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_INCREMENTABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_INCREMENTABLE_HPP

/**
 *  @file algorithm/traits/is_weakly_incrementable.hpp
 *  @brief `is_weakly_incrementable` trait
 *
 *  This file defines the `is_weakly_incrementable` trait.
 */

#include <futures/algorithm/traits/is_movable.hpp>
#include <futures/algorithm/traits/iter_difference.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::weakly_incrementable` concept
    /**
     * @see https://en.cppreference.com/w/cpp/iterator/weakly_incrementable
     */
#ifdef FUTURES_DOXYGEN
    template <class I>
    using is_weakly_incrementable = std::bool_constant<std::weakly_incrementable<T>>;
#else
    template <class I, class = void>
    struct is_weakly_incrementable : std::false_type {};

    template <class I>
    struct is_weakly_incrementable<
        I,
        std::void_t<
            // clang-format off
            decltype(std::declval<I>()++),
            decltype(++std::declval<I>()),
            iter_difference_t<I>
            // clang-format on
            >>
        : std::conjunction<
              // clang-format off
              is_movable<I>,
              std::is_same<decltype(++std::declval<I>()), I&>
              // clang-format on
              > {};
#endif

    /// @copydoc is_weakly_incrementable
    template <class I>
    constexpr bool is_weakly_incrementable_v = is_weakly_incrementable<I>::value;

    /** @} */
    /** @} */

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_INCREMENTABLE_HPP
