//
// Copyright (c) 2023 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_PARTITIONER_THREAD_PARTITIONER_HPP
#define FUTURES_ALGORITHM_PARTITIONER_THREAD_PARTITIONER_HPP

#include <futures/config.hpp>
#include <futures/algorithm/traits/is_input_iterator.hpp>
#include <futures/algorithm/traits/is_sentinel_for.hpp>
#include <futures/executor/hardware_concurrency.hpp>
#include <thread>

/**
 *  @file algorithm/partitioner/thread_partitioner.hpp
 *  @brief Thread Partitioner
 *
 *  Define the thread_partitioner class
 */

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup partitioners Partitioners
     *  @{
     */

    /// A partitioner that always splits the problem when moving to new threads
    /**
     *  A partitioner that splits the ranges until it identifies we are
     *  not moving to new threads.
     *
     *  This partitioner splits the ranges until it identifies we are not moving
     *  to new threads. Apart from that, it behaves as a halve_partitioner,
     *  splitting the range up to a minimum grain size.
     */
    class thread_partitioner {
        std::size_t min_grain_size_;
        std::size_t num_threads_{ hardware_concurrency() };
        std::thread::id last_thread_id_{};

    public:
        explicit thread_partitioner(std::size_t min_grain_size)
            : min_grain_size_(min_grain_size) {}

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
            if (num_threads_ <= 1) {
                return last;
            }
            std::thread::id current_thread_id = std::this_thread::get_id();
            bool const threads_changed = current_thread_id != last_thread_id_;
            if (threads_changed) {
                last_thread_id_ = current_thread_id;
                num_threads_ += 1;
                num_threads_ /= 2;
                std::size_t size = std::distance(first, last);
                return (size <= min_grain_size_) ?
                           last :
                           std::next(first, (size + 1) / 2);
            }
            return last;
        }
    };

    /** @} */ // @addtogroup partitioners Partitioners
    /** @} */ // @addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_ALGORITHM_PARTITIONER_THREAD_PARTITIONER_HPP