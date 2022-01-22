//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_COUNT_IF_H
#define FUTURES_COUNT_IF_H

#include <futures/algorithm/partitioner/partitioner.h>
#include <futures/algorithm/traits/unary_invoke_algorithm.h>
#include <futures/futures.h>
#include <futures/algorithm/detail/traits/range/range/concepts.h>
#include <futures/algorithm/detail/try_async.h>
#include <execution>
#include <variant>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref count_if function
    class count_if_functor
        : public unary_invoke_algorithm_functor<count_if_functor>
    {

        friend unary_invoke_algorithm_functor<count_if_functor>;

        /// \brief Complete overload of the count_if algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param f Function
        /// \brief function template \c count_if
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun,
            std::enable_if_t<
                is_executor_v<
                    E> && is_partitioner_v<P, I, S> && is_input_iterator_v<I> && futures::detail::sentinel_for<S, I> && futures::detail::indirectly_unary_invocable<Fun, I> && std::is_copy_constructible_v<Fun>,
                int> = 0>
        futures::detail::iter_difference_t<I>
        run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last
                || std::is_same_v<
                    E,
                    inline_executor> || futures::detail::forward_iterator<I>)
            {
                return std::count_if(first, last, f);
            }

            // Run count_if on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() {
                return operator()(ex, p, middle, last, f);
            });

            // Run count_if on lhs: [first, middle]
            bool lhs = operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs + rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                return lhs
                       + operator()(make_inline_executor(), p, middle, last, f);
            }
        }
    };

    /// \brief Returns the number of elements satisfying specific criteria
    inline constexpr count_if_functor count_if;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_COUNT_IF_H
