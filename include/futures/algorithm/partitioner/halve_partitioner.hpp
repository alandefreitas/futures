//
// Copyright (c) 2023 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_PARTITIONER_HALVE_PARTITIONER_HPP
#define FUTURES_ALGORITHM_PARTITIONER_HALVE_PARTITIONER_HPP

#include <futures/config.hpp>
#include <futures/algorithm/traits/is_input_iterator.hpp>
#include <futures/algorithm/traits/is_sentinel_for.hpp>
#include <futures/detail/traits/std_type_traits.hpp>

/**
 *  @file algorithm/partitioner/halve_partitioner.hpp
 *  @brief Halve Partitioner
 *
 *  Define the halve_partitioner class
 */

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup partitioners Partitioners
     *  @{
     */

    /// A partitioner that always splits the problem in half
    /**
     *  The halve partitioner always splits the sequence into two parts
     *  of roughly equal size
     *
     *  The sequence is split up to a minimum grain size.
     *  As a concept, the result from the partitioner is considered a suggestion
     *  for parallelization. For algorithms such as for_each, a partitioner with
     *  a very small grain size might be appropriate if the operation is very
     *  expensive. Some algorithms, such as a binary search, might naturally
     *  adjust this suggestion so that the result makes sense.
     */
    class halve_partitioner {
        std::size_t min_grain_size_;

    public:
        /// Constructor
        /**
         * The constructor has a minimum grain size after which the range
         * should not be split.
         *
         * @param min_grain_size_ Minimum grain size used to split ranges
         */
        constexpr explicit halve_partitioner(std::size_t min_grain_size_)
            : min_grain_size_(min_grain_size_) {}

        /// Split a range of elements
        /**
         *  @tparam I Iterator type
         *  @tparam S Sentinel type
         *  @param first First element in range
         *  @param last Last element in range
         *  @return Iterator to point where sequence should be split
         */
#ifdef FUTURES_HAS_CONCEPTS
        template <std::input_iterator I, std::sentinel_for<I> S>
#else
        template <
            class I,
            class S,
            std::enable_if_t<
                is_input_iterator_v<I> && is_sentinel_for_v<S, I>,
                int>
            = 0>
#endif
        I
        operator()(I first, S last) {
            std::size_t size = std::distance(first, last);
            return (size <= min_grain_size_) ?
                       last :
                       std::next(first, (size + 1) / 2);
        }
    };

    /** @} */ // @addtogroup partitioners Partitioners
    /** @} */ // @addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_ALGORITHM_PARTITIONER_HALVE_PARTITIONER_HPP