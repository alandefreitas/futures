//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_NONE_OF_HPP
#define FUTURES_ALGORITHM_NONE_OF_HPP

/**
 *  @file algorithm/none_of.hpp
 *  @brief `none_of` algorithm
 *
 *  This file defines the functor and callable for a parallel version of the
 *  `none_of` algorithm.
 */

#include <futures/future.hpp>
#include <futures/is_ready.hpp>
#include <futures/launch.hpp>
#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/traits/is_forward_iterator.hpp>
#include <futures/algorithm/traits/unary_invoke_algorithm.hpp>
#include <futures/algorithm/detail/execution.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <futures/detail/deps/boost/core/ignore_unused.hpp>
#include <variant>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */
    /** @addtogroup functions Functions
     *  @{
     */


    /// Functor representing the overloads for the @ref none_of function
    class none_of_functor
        : public unary_invoke_algorithm_functor<none_of_functor> {
        friend unary_invoke_algorithm_functor<none_of_functor>;

        template <class Executor>
        class none_of_graph : public boost::empty_value<Executor> {
        public:
            explicit none_of_graph(Executor const &ex)
                : boost::empty_value<Executor>(boost::empty_init, ex) {}

            template <class P, class I, class S, class Fun>
            bool
            launch_none_of_tasks(P p, I first, S last, Fun f) {
                auto middle = p(first, last);
                bool const too_small = middle == last;
                constexpr bool cannot_parallelize
                    = std::is_same_v<Executor, inline_executor>
                      || is_forward_iterator_v<I>;
                if (too_small || cannot_parallelize) {
                    return std::none_of(first, last, f);
                } else {
                    // Create task that launches tasks for rhs: [middle, last]
                    basic_future<
                        bool,
                        future_options<executor_opt<Executor>, continuable_opt>>
                        rhs_task = futures::async(
                            boost::empty_value<Executor>::get(),
                            [this, p, middle, last, f] {
                        return launch_none_of_tasks(p, middle, last, f);
                            });

                    // Launch tasks for lhs: [first, middle]
                    bool lhs_result = launch_none_of_tasks(p, first, middle, f);

                    // When lhs is ready, we check on rhs
                    if (!is_ready(rhs_task)) {
                        // Put rhs_task on the list of tasks we need to await
                        // later. This ensures we only deal with the task queue
                        // if we really need to.
                        if (lhs_result) {
                            tasks_.push(std::move(rhs_task));
                        } else {
                            rhs_task.detach();
                        }
                        return lhs_result;
                    } else {
                        return lhs_result && rhs_task.get();
                    }
                }
            }

            bool
            wait_for_none_of_tasks() {
                while (!tasks_.empty()) {
                    if (!tasks_.pop().get()) {
                        return false;
                    }
                }
                return true;
            }

            template <class P, class I, class S, class Fun>
            bool
            none_of(P p, I first, S last, Fun f) {
                bool partial = launch_none_of_tasks(p, first, last, f);
                if (partial) {
                    return wait_for_none_of_tasks();
                } else {
                    return false;
                }
            }

        private:
            detail::atomic_queue<basic_future<
                bool,
                future_options<executor_opt<Executor>, continuable_opt>>>
                tasks_{};
        };

        template <
            class I,
            class S,
            class Fun FUTURES_REQUIRE(
                (is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                 && is_indirectly_unary_invocable_v<Fun, I>
                 && std::is_copy_constructible_v<Fun>) )>
        static FUTURES_CONSTANT_EVALUATED_CONSTEXPR bool
        inline_none_of(I first, S last, Fun p) {
            for (; first != last; ++first) {
                if (p(*first)) {
                    return false;
                }
            }
            return true;
        }

        /// Complete overload of the none_of algorithm
        /**
         *  @tparam E Executor type
         *  @tparam P Partitioner type
         *  @tparam I Iterator type
         *  @tparam S Sentinel iterator type
         *  @tparam Fun Function type
         *  @param ex Executor
         *  @param p Partitioner
         *  @param first Iterator to first element in the range
         *  @param last Iterator to (last + 1)-th element in the range
         *  @param f Function
         *  function template \c none_of
         */
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun FUTURES_REQUIRE((
                is_executor_v<E> && is_partitioner_v<P, I, S>
                && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                && is_indirectly_unary_invocable_v<Fun, I>
                && std::is_copy_constructible_v<Fun>) )>
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR bool
        run(E const &ex, P p, I first, S last, Fun f) const {
            if constexpr (std::is_same_v<std::decay_t<E>, inline_executor>) {
                boost::ignore_unused(p);
                return inline_none_of(first, last, f);
            } else {
                if (detail::is_constant_evaluated()) {
                    boost::ignore_unused(p);
                    return inline_none_of(first, last, f);
                } else {
                    return none_of_graph<E>(ex).none_of(p, first, last, f);
                }
            }
        }
    };

    /// Checks if a predicate is true for none of the elements in a range
    inline constexpr none_of_functor none_of;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_NONE_OF_HPP
