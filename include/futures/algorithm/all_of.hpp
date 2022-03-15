
//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_ALL_OF_HPP
#define FUTURES_ALGORITHM_ALL_OF_HPP

#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/traits/is_forward_iterator.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/is_sentinel_for.hpp>
#include <futures/algorithm/traits/unary_invoke_algorithm.hpp>
#include <futures/futures.hpp>
#include <futures/detail/container/atomic_queue.hpp>
#include <futures/executor/detail/maybe_empty_executor.hpp>
#include <execution>
#include <variant>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup functions Functions
     *  @{
     */

    /// Functor representing the overloads for the @ref all_of function
    class all_of_functor : public unary_invoke_algorithm_functor<all_of_functor>
    {
        friend unary_invoke_algorithm_functor<all_of_functor>;

        template <class Executor>
        class all_of_graph : public detail::maybe_empty_executor<Executor>
        {
        public:
            explicit all_of_graph(const Executor &ex)
                : detail::maybe_empty_executor<Executor>(ex) {}

            template <class P, class I, class S, class Fun>
            bool
            launch_all_of_tasks(P p, I first, S last, Fun f) {
                auto middle = p(first, last);
                const bool too_small = middle == last;
                constexpr bool cannot_parallelize
                    = std::is_same_v<
                          Executor,
                          inline_executor> || is_forward_iterator_v<I>;
                if (too_small || cannot_parallelize) {
                    return std::all_of(first, last, f);
                } else {
                    // Create task that launches tasks for rhs: [middle, last]
                    basic_future<
                        bool,
                        future_options<executor_opt<Executor>, continuable_opt>>
                        rhs_task = futures::async(
                            this->get_executor(),
                            [this, p, middle, last, f] {
                        return launch_all_of_tasks(p, middle, last, f);
                            });

                    // Launch tasks for lhs: [first, middle]
                    bool lhs_result = launch_all_of_tasks(p, first, middle, f);

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
            wait_for_all_of_tasks() {
                while (!tasks_.empty()) {
                    if (!tasks_.pop().get()) {
                        return false;
                    }
                }
                return true;
            }

            template <class P, class I, class S, class Fun>
            bool
            all_of(P p, I first, S last, Fun f) {
                bool partial = launch_all_of_tasks(p, first, last, f);
                if (partial) {
                    return wait_for_all_of_tasks();
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


        /// Complete overload of the all_of algorithm
        /// @tparam E Executor type
        /// @tparam P Partitioner type
        /// @tparam I Iterator type
        /// @tparam S Sentinel iterator type
        /// @tparam Fun Function type
        /// @param ex Executor
        /// @param p Partitioner
        /// @param first Iterator to first element in the range
        /// @param last Iterator to (last + 1)-th element in the range
        /// @param f Function
        /// function template \c all_of
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
            return all_of_graph<E>(ex).all_of(p, first, last, f);
        }
    };

    /// Checks if a predicate is true for all the elements in a range
    inline constexpr all_of_functor all_of;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_ALL_OF_HPP
