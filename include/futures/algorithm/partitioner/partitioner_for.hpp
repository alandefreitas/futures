//
// Copyright (c) 2023 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_PARTITIONER_PARTITIONER_FOR_HPP
#define FUTURES_ALGORITHM_PARTITIONER_PARTITIONER_FOR_HPP

#include <futures/config.hpp>
#include <futures/algorithm/traits/is_input_iterator.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/utility/invoke.hpp>

/**
 *  @file algorithm/partitioner/partitioner_for.hpp
 *  @brief Concepts and traits for partitioners
 *
 *  A partitioner is a light callable object that takes a pair of iterators and
 *  returns the middle of the sequence. In particular, it returns an iterator
 *  `middle` that forms a subrange `first`/`middle` which the algorithm should
 *  solve inline before scheduling the subrange `middle`/`last` in the executor.
 */

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup partitioners Partitioners
     *  @{
     */

    /// Determine if P is a valid partitioner for the iterator range [I,S]
    template <class T, class I, class S = I>
    using is_partitioner_for = detail::conjunction<
        std::conditional_t<
            is_input_iterator_v<I>,
            std::true_type,
            std::false_type>,
        std::conditional_t<
            is_input_iterator_v<S>,
            std::true_type,
            std::false_type>,
        detail::is_invocable<T, I, S>>;

    /// Determine if P is a valid partitioner for the iterator range [I,S]
    template <class P, class I, class S = I>
    constexpr bool is_partitioner_for_v = is_partitioner_for<P, I, S>::value;

#ifdef FUTURES_HAS_CONCEPTS
    /// @concept partitioner_for
    /// @brief Determines if a type is an partitioner
    template <class P, class I, class S = I>
    concept partitioner_for = is_partitioner_for_v<P, I, S>;
#endif

    /** @} */ // @addtogroup partitioners Partitioners
    /** @} */ // @addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_ALGORITHM_PARTITIONER_PARTITIONER_FOR_HPP