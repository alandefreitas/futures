//
// Copyright (c) 2023 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_PARTITIONER_DEFAULT_PARTITIONER_HPP
#define FUTURES_ALGORITHM_PARTITIONER_DEFAULT_PARTITIONER_HPP

#include <futures/config.hpp>
#include <futures/algorithm/partitioner/thread_partitioner.hpp>
#include <futures/algorithm/traits/is_input_range.hpp>

/**
 *  @file algorithm/partitioner/default_partitioner.hpp
 *  @brief Default Partitioner
 *
 *  Define the default partitioner
 */

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup partitioners Partitioners
     *  @{
     */

    /// Default partitioner used by parallel algorithms
    /**
     *  Its type and parameters might change
     */
    using default_partitioner = FUTURES_DETAIL(thread_partitioner);

    /// Determine a reasonable minimum grain size depending on the number
    /// of elements in a sequence
    /**
     * The grain size considers the number of threads available.
     * It's never more than 2048 elements.
     *
     * @param n Sequence size
     * @return The recommended grain size for a range of the specified size
     */
    FUTURES_CONSTANT_EVALUATED_CONSTEXPR std::size_t
    make_grain_size(std::size_t n) {
        std::size_t const nthreads = futures::hardware_concurrency();
        std::size_t const safe_nthreads = std::
            max(nthreads, static_cast<std::size_t>(1));
        std::size_t const expected_nthreads = 8 * safe_nthreads;
        std::size_t const grain_per_thread = n / expected_nthreads;
        return grain_per_thread < size_t(1) ?
                   size_t(1) :
               size_t(2048) < grain_per_thread ?
                   size_t(2048) :
                   grain_per_thread;
    }

    /// Create an instance of the default partitioner with a reasonable
    /// grain size for `n` elements
    /**
     *  The default partitioner type and parameters might change
     */
    inline default_partitioner
    make_default_partitioner(size_t n) {
        return default_partitioner(make_grain_size(n));
    }

    /// Create an instance of the default partitioner with a reasonable
    /// grain for the range `first`, `last`
    /**
     *  The default partitioner type and parameters might change
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <std::input_iterator I, std::sentinel_for<I> S>
#else
    template <
        class I,
        class S,
        std::enable_if_t<is_input_iterator_v<I> && is_sentinel_for_v<S, I>, int>
        = 0>
#endif
    default_partitioner
    make_default_partitioner(I first, S last) {
        return make_default_partitioner(std::distance(first, last));
    }

    /// Create an instance of the default partitioner with a reasonable
    /// grain for the range `r`
    /**
     *  The default partitioner type and parameters might change
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <class R>
    requires is_input_range_v<R>
#else
    template <class R, std::enable_if_t<is_input_range_v<R>, int> = 0>
#endif
    default_partitioner
    make_default_partitioner(R &&r) {
        return make_default_partitioner(std::begin(r), std::end(r));
    }

    /** @} */ // @addtogroup partitioners Partitioners
    /** @} */ // @addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_ALGORITHM_PARTITIONER_DEFAULT_PARTITIONER_HPP