//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_COUNT_H
#define FUTURES_COUNT_H

#include <futures/algorithm/comparisons/equal_to.hpp>
#include <futures/algorithm/count_if.hpp>
#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/traits/is_forward_iterator.hpp>
#include <futures/algorithm/traits/is_indirectly_binary_invocable.hpp>
#include <futures/algorithm/traits/iter_difference.hpp>
#include <futures/algorithm/traits/value_cmp_algorithm.hpp>
#include <futures/futures.hpp>
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
            return count_if_functor::count_if_graph<E, I>(ex)
                .count_if(p, first, last, [&v](const auto &el) {
                    return el == v;
                });
        }
    };

    /// \brief Returns the number of elements matching an element
    inline constexpr count_functor count;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_COUNT_H