//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_REDUCE_H
#define FUTURES_REDUCE_H

#include <futures/algorithm/partitioner/partitioner.h>
#include <futures/algorithm/traits/binary_invoke_algorithm.h>
#include <futures/futures.h>
#include <futures/algorithm/detail/try_async.h>
#include <execution>
#include <numeric>
#include <variant>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */
    /** \addtogroup functions Functions
     *  @{
     */


    /// \brief Functor representing the overloads for the @ref reduce function
    class reduce_functor
        : public binary_invoke_algorithm_functor<reduce_functor>
    {
        friend binary_invoke_algorithm_functor<reduce_functor>;

        /// \brief Complete overload of the reduce algorithm
        ///
        /// The reduce algorithm is equivalent to a version std::accumulate
        /// where the binary operation is applied out of order. \tparam E
        /// Executor type \tparam P Partitioner type \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param i Initial value for the reduction
        /// \param f Function
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                std::is_same_v<iter_value_t<I>, T> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>,
                // clang-format on
                int> = 0
#endif
            >
        T
        run(const E &ex, P p, I first, S last, T i, Fun f = std::plus<>())
            const {
            auto middle = p(first, last);
            if (middle == last
                || std::is_same_v<
                    E,
                    inline_executor> || is_forward_iterator_v<I>)
            {
                return std::reduce(first, last, i, f);
            }

            // Run reduce on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() {
                return operator()(ex, p, middle, last, i, f);
            });

            // Run reduce on lhs: [first, middle]
            T lhs = operator()(ex, p, first, middle, i, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return f(lhs, rhs.get());
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                T i_rhs =
                operator()(make_inline_executor(), p, middle, last, i, f);
                return f(lhs, i_rhs);
            }
        }
    };

    /// \brief Sums up (or accumulate with a custom function) a range of
    /// elements, except out of order
    inline constexpr reduce_functor reduce;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_REDUCE_H
