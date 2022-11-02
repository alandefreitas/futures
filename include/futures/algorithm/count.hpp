//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COUNT_HPP
#define FUTURES_ALGORITHM_COUNT_HPP

/**
 *  @file algorithm/count.hpp
 *  @brief `count` algorithm
 *
 *  This file defines the functor and callable for a parallel version of the
 *  `count` algorithm.
 */

#include <futures/algorithm/comparisons/equal_to.hpp>
#include <futures/algorithm/count_if.hpp>
#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/traits/is_forward_iterator.hpp>
#include <futures/algorithm/traits/is_indirectly_binary_invocable.hpp>
#include <futures/algorithm/traits/iter_difference.hpp>
#include <futures/algorithm/traits/value_cmp_algorithm.hpp>
#include <futures/algorithm/detail/execution.hpp>
#include <futures/detail/deps/boost/core/ignore_unused.hpp>
#include <variant>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup functions Functions
     *  @{
     */


    /// Functor representing the overloads for the @ref count function
    class count_functor : public value_cmp_algorithm_functor<count_functor> {
        friend value_cmp_algorithm_functor<count_functor>;

        template <
            class I,
            class S,
            class T FUTURES_REQUIRE(
                (is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                 && is_indirectly_binary_invocable_v<equal_to, T *, I>) )>
        static FUTURES_CONSTANT_EVALUATED_CONSTEXPR iter_difference_t<I>
        inline_count(I first, S last, T const &v) {
            iter_difference_t<I> ret = 0;
            for (; first != last; ++first) {
                if (*first == v) {
                    ret++;
                }
            }
            return ret;
        }


        /// Complete overload of the count algorithm
        /**
         *  @tparam E Executor type
         *  @tparam P Partitioner type
         *  @tparam I Iterator type
         *  @tparam S Sentinel iterator type
         *  @tparam T Value to compare
         *
         *  @param ex Executor
         *  @param p Partitioner
         *  @param first Iterator to first element in the range
         *  @param last Iterator to (last + 1)-th element in the range
         *  @param v Value
         *  function template \c count
         */
        template <
            class E,
            class P,
            class I,
            class S,
            class T FUTURES_REQUIRE(
                (is_executor_v<E> && is_partitioner_v<P, I, S>
                 && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                 && is_indirectly_binary_invocable_v<equal_to, T *, I>) )>
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR iter_difference_t<I>
        run(E const &ex, P p, I first, S last, T const &v) const {
            if constexpr (std::is_same_v<std::decay_t<E>, inline_executor>) {
                boost::ignore_unused(p);
                return inline_count(first, last, v);
            } else {
                if (detail::is_constant_evaluated()) {
                    boost::ignore_unused(p);
                    return inline_count(first, last, v);
                } else {
                    return count_if_functor::count_if_graph<E, I>(ex)
                        .count_if(p, first, last, [&v](auto const &el) {
                            return el == v;
                        });
                }
            }
        }
    };

    /// Returns the number of elements matching an element
    inline constexpr count_functor count;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_COUNT_HPP
