//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_COUNT_H
#define FUTURES_COUNT_H

#include <futures/algorithm/comparisons/equal_to.h>
#include <futures/algorithm/partitioner/partitioner.h>
#include <futures/algorithm/traits/is_forward_iterator.h>
#include <futures/algorithm/traits/is_indirectly_binary_invocable.h>
#include <futures/algorithm/traits/iter_difference.h>
#include <futures/algorithm/traits/value_cmp_algorithm.h>
#include <futures/futures.h>
#include <futures/algorithm/detail/try_async.h>
#include <execution>
#include <variant>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup functions Functions
     *  @{
     */


    /// \brief Functor representing the overloads for the @ref count function
    class count_functor : public value_cmp_algorithm_functor<count_functor>
    {
        friend value_cmp_algorithm_functor<count_functor>;

        /// \brief Complete overload of the count algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam T Value to compare
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param value Value
        /// \brief function template \c count
        template <
            class E,
            class P,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<equal_to, T *, I>
                // clang-format on
                ,
                int> = 0
#endif
            >
        iter_difference_t<I>
        run(const E &ex, P p, I first, S last, T v) const {
            auto middle = p(first, last);
            if (middle == last
                || std::
                    is_same_v<E, inline_executor> || is_forward_iterator_v<I>)
            {
                return std::count(first, last, v);
            }

            // Run count on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel]
                = try_async(ex, [ex, p, middle, last, v, this]() {
                      return operator()(ex, p, middle, last, v);
                  });

            // Run count on lhs: [first, middle]
            auto lhs = operator()(ex, p, first, middle, v);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs + rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                return lhs
                       + operator()(make_inline_executor(), p, middle, last, v);
            }
        }
    };

    /// \brief Returns the number of elements matching an element
    inline constexpr count_functor count;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_COUNT_H
