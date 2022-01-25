//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_NONE_OF_H
#define FUTURES_NONE_OF_H

#include <futures/algorithm/partitioner/partitioner.h>
#include <futures/algorithm/traits/unary_invoke_algorithm.h>
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


    /// \brief Functor representing the overloads for the @ref none_of function
    class none_of_functor
        : public unary_invoke_algorithm_functor<none_of_functor>
    {
        friend unary_invoke_algorithm_functor<none_of_functor>;

        /// \brief Complete overload of the none_of algorithm
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
        /// \brief function template \c none_of
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        bool
        run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last
                || std::is_same_v<
                    E,
                    inline_executor> || is_forward_iterator_v<I>)
            {
                return std::none_of(first, last, f);
            }

            // Run none_of on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel]
                = try_async(ex, [ex, p, middle, last, f, this]() {
                      return operator()(ex, p, middle, last, f);
                  });

            // Run none_of on lhs: [first, middle]
            bool lhs = operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs && rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                if (!lhs) {
                    return false;
                } else {
                    return
                    operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Checks if a predicate is true for none of the elements in a range
    inline constexpr none_of_functor none_of;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_NONE_OF_H
