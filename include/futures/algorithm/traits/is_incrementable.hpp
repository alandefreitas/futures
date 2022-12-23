//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INCREMENTABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_INCREMENTABLE_HPP

/**
 *  @file algorithm/traits/is_incrementable.hpp
 *  @brief `is_incrementable` trait
 *
 *  This file defines the `is_incrementable` trait.
 */

#include <futures/algorithm/traits/is_regular.hpp>
#include <futures/algorithm/traits/is_weakly_incrementable.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::incrementable` concept
    /**
     * @see https://en.cppreference.com/w/cpp/iterator/incrementable
     */
#ifdef FUTURES_DOXYGEN
    template <class I>
    using is_incrementable = std::bool_constant<std::incrementable<I>>;
#else
    template <class I, class = void>
    struct is_incrementable : std::false_type {};

    template <class I>
    struct is_incrementable<
        I,
        detail::void_t<
            // clang-format off
            decltype(std::declval<I&>()++)
            // clang-format on
            >>
        : detail::conjunction<
              // clang-format off
              is_regular<I>,
              is_weakly_incrementable<I>,
              std::is_same<decltype(std::declval<I&>()++), I>
              // clang-format on
              > {};
#endif

    /// @copydoc is_incrementable
    template <class I>
    constexpr bool is_incrementable_v = is_incrementable<I>::value;

    /** @} */
    /** @} */

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INCREMENTABLE_HPP
