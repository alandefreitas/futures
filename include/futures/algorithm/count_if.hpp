//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COUNT_IF_HPP
#define FUTURES_ALGORITHM_COUNT_IF_HPP

#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/traits/unary_invoke_algorithm.hpp>
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

    class count_functor;

    /// \brief Functor representing the overloads for the @ref count_if function
    class count_if_functor
        : public unary_invoke_algorithm_functor<count_if_functor>
    {
        friend unary_invoke_algorithm_functor<count_if_functor>;
        friend count_functor;

        template <class Executor, class I>
        class count_if_graph : public detail::maybe_empty<Executor>
        {
        public:
            explicit count_if_graph(const Executor &ex)
                : detail::maybe_empty<Executor>(ex) {}

            template <class P, class S, class Fun>
            iter_difference_t<I>
            launch_count_if_tasks(P p, I first, S last, Fun f) {
                auto middle = p(first, last);
                const iter_difference_t<I> too_small = middle == last;
                constexpr iter_difference_t<I> cannot_parallelize
                    = std::is_same_v<
                          Executor,
                          inline_executor> || is_forward_iterator_v<I>;
                if (too_small || cannot_parallelize) {
                    return std::count_if(first, last, f);
                } else {
                    // Create task that launches tasks for rhs: [middle, last]
                    basic_future<
                        iter_difference_t<I>,
                        future_options<executor_opt<Executor>, continuable_opt>>
                        rhs_task = futures::async(
                            detail::maybe_empty<Executor>::get(),
                            [this, p, middle, last, f] {
                        return launch_count_if_tasks(p, middle, last, f);
                            });

                    // Launch tasks for lhs: [first, middle]
                    iter_difference_t<I>
                        lhs_result = launch_count_if_tasks(p, first, middle, f);

                    // When lhs is ready, we check on rhs
                    if (!is_ready(rhs_task)) {
                        // Put rhs_task on the list of tasks we need to await
                        // later. This ensures we only deal with the task queue
                        // if we really need to.
                        tasks_.push(std::move(rhs_task));
                        return lhs_result;
                    } else {
                        return lhs_result + rhs_task.get();
                    }
                }
            }

            iter_difference_t<I>
            wait_for_count_if_tasks() {
                iter_difference_t<I> sum = 0;
                while (!tasks_.empty()) {
                    sum += tasks_.pop().get();
                }
                return sum;
            }

            template <class P, class S, class Fun>
            iter_difference_t<I>
            count_if(P p, I first, S last, Fun f) {
                iter_difference_t<I>
                    partial = launch_count_if_tasks(p, first, last, f);
                return partial + wait_for_count_if_tasks();
            }

        private:
            detail::atomic_queue<basic_future<
                iter_difference_t<I>,
                future_options<executor_opt<Executor>, continuable_opt>>>
                tasks_{};
        };

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
        iter_difference_t<I>
        run(const E &ex, P p, I first, S last, Fun f) const {
            return count_if_graph<E, I>(ex).count_if(p, first, last, f);
        }
    };

    /// \brief Returns the number of elements satisfying specific criteria
    inline constexpr count_if_functor count_if;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_COUNT_IF_HPP
