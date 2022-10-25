//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_FORWARD_ITERATOR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_FORWARD_ITERATOR_HPP

/**
 *  @file algorithm/traits/is_forward_iterator.hpp
 *  @brief `is_forward_iterator` trait
 *
 *  This file defines the `is_forward_iterator` trait.
 */

#include <futures/algorithm/traits/is_derived_from.hpp>
#include <futures/algorithm/traits/is_incrementable.hpp>
#include <futures/algorithm/traits/is_input_iterator.hpp>
#include <futures/algorithm/traits/is_sentinel_for.hpp>
#include <futures/algorithm/traits/detail/iter_concept.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::forward_iterator` concept
    /**
     * @see https://en.cppreference.com/w/cpp/iterator/forward_iterator
     */
    template <class I>
    using is_forward_iterator = std::conjunction<
        is_input_iterator<I>,
        is_derived_from<detail::iter_concept_t<I>, std::forward_iterator_tag>,
        is_incrementable<I>,
        is_sentinel_for<I, I>>;

    /// @copydoc is_forward_iterator
    template <class I>
    constexpr bool is_forward_iterator_v = is_forward_iterator<I>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_FORWARD_ITERATOR_HPP
