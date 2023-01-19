//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_RANDOM_ACCESS_ITERATOR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_RANDOM_ACCESS_ITERATOR_HPP

/**
 *  @file algorithm/traits/is_random_access_iterator.hpp
 *  @brief `is_random_access_iterator` trait
 *
 *  This file defines the `is_random_access_iterator` trait.
 */

#include <futures/config.hpp>
#include <futures/algorithm/traits/is_bidirectional_iterator.hpp>
#include <futures/algorithm/traits/is_totally_ordered.hpp>
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

    /// @brief A type trait equivalent to the `std::random_access_iterator`
    /// concept
    /**
     * @see
     * [`std::random_access_iterator`](https://en.cppreference.com/w/cpp/iterator/random_access_iterator)
     */
#if defined(FUTURES_DOXYGEN)
    template <class T>
    using is_random_access_iterator = std::bool_constant<
        std::random_access_iterator<T>>;
#else
    template <class I, class = void>
    struct is_random_access_iterator : std::false_type {};

    template <class I>
    struct is_random_access_iterator<
        I,
        detail::void_t<
            // clang-format off
            decltype(std::declval<I&>() += std::declval<iter_difference_t<I>>()),
            decltype(std::declval<I const&>() + std::declval<iter_difference_t<I>>()),
            decltype(std::declval<iter_difference_t<I>>() + std::declval<I const&>()),
            decltype(std::declval<I&>() -= std::declval<iter_difference_t<I>>()),
            decltype(std::declval<I const&>() - std::declval<iter_difference_t<I>>()),
            decltype(std::declval<I const&>()[std::declval<iter_difference_t<I>>()])
            // clang-format on
            >>
        : detail::conjunction<
              // clang-format off
              is_bidirectional_iterator<I>,
              is_derived_from<detail::iter_concept_t<I>, std::random_access_iterator_tag>,
              is_totally_ordered<I>,
              is_sentinel_for<I, I>,
              std::is_same<decltype(std::declval<I&>() += std::declval<iter_difference_t<I>>()), I&>,
              std::is_same<decltype(std::declval<I const&>() + std::declval<iter_difference_t<I>>()), I>,
              std::is_same<decltype(std::declval<iter_difference_t<I>>() + std::declval<I const&>()), I>,
              std::is_same<decltype(std::declval<I&>() -= std::declval<iter_difference_t<I>>()), I&>,
              std::is_same<decltype(std::declval<I const&>() - std::declval<iter_difference_t<I>>()), I>,
              std::is_same<decltype(std::declval<I const&>()[std::declval<iter_difference_t<I>>()]), iter_reference_t<I>>
              // clang-format on
              > {};
#endif

    /// @copydoc is_random_access_iterator
    template <class I>
    constexpr bool is_random_access_iterator_v = is_random_access_iterator<
        I>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_RANDOM_ACCESS_ITERATOR_HPP
