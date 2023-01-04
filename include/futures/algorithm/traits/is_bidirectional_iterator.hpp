//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_BIDIRECTIONAL_ITERATOR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_BIDIRECTIONAL_ITERATOR_HPP

/**
 *  @file algorithm/traits/is_bidirectional_iterator.hpp
 *  @brief `is_bidirectional_iterator` trait
 *
 *  This file defines the `is_bidirectional_iterator` trait.
 */

#include <futures/config.hpp>
#include <futures/algorithm/traits/is_forward_iterator.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/algorithm/traits/detail/iter_concept.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::bidirectional_iterator`
    /// concept
    /**
     * @see [`std::bidirectional_iterator`](https://en.cppreference.com/w/cpp/iterator/bidirectional_iterator)
     */
#if defined(FUTURES_DOXYGEN)
    template <class T>
    using is_bidirectional_iterator = std::bool_constant<
        std::bidirectional_iterator<T>>;
#else
    template <class I, class = void>
    struct is_bidirectional_iterator : std::false_type {};

    template <class I>
    struct is_bidirectional_iterator<
        I,
        detail::void_t<
            // clang-format off
            decltype(std::declval<I&>()--),
            decltype(--std::declval<I&>())
            // clang-format on
            >>
        : detail::conjunction<
              // clang-format off
              is_forward_iterator<I>,
              is_derived_from<detail::iter_concept_t<I>, std::bidirectional_iterator_tag>,
              std::is_same<decltype(--std::declval<I&>()), I&>,
              std::is_same<decltype(std::declval<I&>()--), I>
              // clang-format on
              > {};
#endif

    /// @copydoc is_bidirectional_iterator
    template <class I>
    constexpr bool is_bidirectional_iterator_v = is_bidirectional_iterator<
        I>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_BIDIRECTIONAL_ITERATOR_HPP
