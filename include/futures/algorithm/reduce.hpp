//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_REDUCE_HPP
#define FUTURES_ALGORITHM_REDUCE_HPP

#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/traits/binary_invoke_algorithm.hpp>
#include <futures/futures.hpp>
#include <execution>
#include <numeric>
#include <variant>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */
    /** @addtogroup functions Functions
     *  @{
     */


    /// Functor representing the overloads for the @ref reduce function
    class reduce_functor
        : public binary_invoke_algorithm_functor<reduce_functor>
    {
        friend binary_invoke_algorithm_functor<reduce_functor>;

        template <class Executor, class T>
        class reduce_graph : public detail::maybe_empty<Executor>
        {
        public:
            explicit reduce_graph(const Executor &ex)
                : detail::maybe_empty<Executor>(ex) {}

            template <class P, class I, class S, class Fun>
            T
            launch_reduce_tasks(P p, I first, S last, T i, Fun f) {
                auto middle = p(first, last);
                const iter_difference_t<I> too_small = middle == last;
                constexpr iter_difference_t<I> cannot_parallelize
                    = std::is_same_v<
                          Executor,
                          inline_executor> || is_forward_iterator_v<I>;
                if (too_small || cannot_parallelize) {
                    return std::reduce(first, last, i, f);
                } else {
                    // Create task that launches tasks for rhs: [middle, last]
                    basic_future<
                        T,
                        future_options<executor_opt<Executor>, continuable_opt>>
                        rhs_task = futures::async(
                            detail::maybe_empty<Executor>::get(),
                            [this, p, middle, last, i, f] {
                        return launch_reduce_tasks(p, middle, last, i, f);
                            });

                    // Launch tasks for lhs: [first, middle]
                    T lhs_result = launch_reduce_tasks(p, first, middle, i, f);

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

            template <class P, class I, class S, class Fun>
            T
            reduce(P p, I first, S last, T i, Fun f) {
                i = launch_reduce_tasks(p, first, last, i, f);
                while (!tasks_.empty()) {
                    i = f(i, tasks_.pop().get());
                }
                return i;
            }

        private:
            detail::atomic_queue<basic_future<
                T,
                future_options<executor_opt<Executor>, continuable_opt>>>
                tasks_{};
        };

        /// Complete overload of the reduce algorithm
        ///
        /// The reduce algorithm is equivalent to a version std::accumulate
        /// where the binary operation is applied out of order. @tparam E
        /// Executor type @tparam P Partitioner type @tparam I Iterator type
        /// @tparam S Sentinel iterator type
        /// @tparam Fun Function type
        /// @param ex Executor
        /// @param p Partitioner
        /// @param first Iterator to first element in the range
        /// @param last Iterator to (last + 1)-th element in the range
        /// @param i Initial value for the reduction
        /// @param f Function
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
            return reduce_graph<E, T>(ex).reduce(p, first, last, i, f);
        }
    };

    /// Sums up (or accumulate with a custom function) a range of
    /// elements, except out of order
    inline constexpr reduce_functor reduce;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_REDUCE_HPP
