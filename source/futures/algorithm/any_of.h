//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ANY_OF_H
#define FUTURES_ANY_OF_H

#include <futures/algorithm/partitioner/partitioner.h>
#include <futures/algorithm/traits/is_forward_iterator.h>
#include <futures/algorithm/traits/unary_invoke_algorithm.h>
#include <futures/futures.h>
#include <execution>
#include <variant>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */
    /** \addtogroup functions Functions
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref any_of function
    class any_of_functor : public unary_invoke_algorithm_functor<any_of_functor>
    {
        friend unary_invoke_algorithm_functor<any_of_functor>;

        template <class Executor>
        class any_of_graph : public detail::maybe_empty<Executor>
        {
        public:
            explicit any_of_graph(const Executor &ex)
                : detail::maybe_empty<Executor>(ex) {}

            template <class P, class I, class S, class Fun>
            bool
            launch_any_of_tasks(P p, I first, S last, Fun f) {
                auto middle = p(first, last);
                const bool too_small = middle == last;
                constexpr bool cannot_parallelize
                    = std::is_same_v<
                          Executor,
                          inline_executor> || is_forward_iterator_v<I>;
                if (too_small || cannot_parallelize) {
                    return std::any_of(first, last, f);
                } else {
                    // Create task that launches tasks for rhs: [middle, last]
                    cfuture<bool> rhs_task = futures::async(
                        detail::maybe_empty<Executor>::get(),
                        [this, p, middle, last, f] {
                        return launch_any_of_tasks(p, middle, last, f);
                        });

                    // Launch tasks for lhs: [first, middle]
                    bool lhs_result = launch_any_of_tasks(p, first, middle, f);

                    // When lhs is ready, we check on rhs
                    if (!is_ready(rhs_task)) {
                        // Put rhs_task on the list of tasks we need to await
                        // later. This ensures we only deal with the task queue
                        // if we really need to.
                        if (lhs_result) {
                            rhs_task.detach();
                        } else {
                            tasks_.push(std::move(rhs_task));
                        }
                        return lhs_result;
                    } else {
                        return lhs_result || rhs_task.get();
                    }
                }
            }

            bool
            wait_for_any_of_tasks() {
                while (!tasks_.empty()) {
                    if (tasks_.pop().get()) {
                        return true;
                    }
                }
                return false;
            }

            template <class P, class I, class S, class Fun>
            bool
            any_of(P p, I first, S last, Fun f) {
                bool partial = launch_any_of_tasks(p, first, last, f);
                if (partial) {
                    return true;
                } else {
                    return wait_for_any_of_tasks();
                }
            }

        private:
            detail::lock_free_queue<cfuture<bool>> tasks_{};
        };

        /// \brief Complete overload of the any_of algorithm
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
        /// \brief function template \c any_of
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
                std::is_copy_constructible_v<Fun>,
                // clang-format on
                int> = 0
#endif
            >
        bool
        run(const E &ex, P p, I first, S last, Fun f) const {
            return any_of_graph<E>(ex).any_of(p, first, last, f);
        }
    };

    /// \brief Checks if a predicate is true for any of the elements in a range
    inline constexpr any_of_functor any_of;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ANY_OF_H
