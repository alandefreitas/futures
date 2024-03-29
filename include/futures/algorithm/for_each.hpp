//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_FOR_EACH_HPP
#define FUTURES_ALGORITHM_FOR_EACH_HPP

/**
 *  @file algorithm/for_each.hpp
 *  @brief `for_each` algorithm
 *
 *  This file defines the functor and callable for a parallel version of the
 *  `for_each` algorithm.
 */

#include <futures/future.hpp>
#include <futures/is_ready.hpp>
#include <futures/launch.hpp>
#include <futures/algorithm/partitioner/partitioner_for.hpp>
#include <futures/algorithm/traits/is_forward_iterator.hpp>
#include <futures/algorithm/traits/unary_invoke_algorithm.hpp>
#include <futures/detail/container/atomic_queue.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/utility/is_constant_evaluated.hpp>
#include <futures/algorithm/detail/execution.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <futures/detail/deps/boost/core/ignore_unused.hpp>


namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */
    /** @addtogroup functions Functions
     *  @{
     */

    /// Functor representing the overloads for the @ref for_each function
    class for_each_functor
        :
#ifdef FUTURES_DOXYGEN
        public unary_invoke_algorithm_functor
#else
        public unary_invoke_algorithm_functor<for_each_functor>
#endif
    {
        friend unary_invoke_algorithm_functor<for_each_functor>;

        /// Internal class that takes care of the sorting tasks and its
        /// incomplete tasks
        /**
         * If we could make sure no executors would ever block, recursion
         * wouldn't be a problem, and we wouldn't need this class. In fact,
         * this is what most related libraries do, counting on the executor to
         * be some kind of work stealing thread pool.
         *
         * However, we cannot count on that, or these algorithms wouldn't work
         * for many executors in which we are interested, such as an io_context
         * or a thread pool that doesn't steel work (like asio's). So we need
         * to separate the process of launching the tasks from the process of
         * waiting for them.
         *
         * Fortunately, we can count that most executors wouldn't need this
         * blocking procedure very often, because that's what usually makes
         * them useful executors. We also assume that, unlike in the other
         * applications, the cost of this reading lock is trivial
         * compared to the cost of the whole procedure and .
         */
        template <class Executor>
        class sort_graph : public boost::empty_value<Executor> {
        public:
            explicit sort_graph(Executor const &ex)
                : boost::empty_value<Executor>(boost::empty_init, ex) {}

            template <class P, class I, class S, class Fun>
            void
            launch_sort_tasks(P p, I first, S last, Fun f) {
                auto middle = p(first, last);
                bool const too_small = middle == last;
                constexpr bool cannot_parallelize
                    = detail::is_same_v<Executor, inline_executor>
                      || is_forward_iterator_v<I>;
                if (too_small || cannot_parallelize) {
                    std::for_each(first, last, f);
                } else {
                    // Create task that launches tasks for rhs: [middle, last]
                    auto rhs_task = futures::async(
                        boost::empty_value<Executor>::get(),
                        [this, p, middle, last, f] {
                        launch_sort_tasks(p, middle, last, f);
                        });

                    // Launch tasks for lhs: [first, middle]
                    launch_sort_tasks(p, first, middle, f);

                    // When lhs is ready, we check on rhs
                    if (!is_ready(rhs_task)) {
                        // Put rhs_task on the list of tasks we need to await
                        // later. This ensures we only deal with the task queue
                        // if we really need to.
                        tasks_.push(std::move(rhs_task));
                    }
                }
            }

            /// Wait for all tasks to finish
            /**
             *  This might sound like it should be as simple as a
             *  when_all(tasks_). However, while we wait for some tasks here,
             *  the running tasks might be enqueuing more tasks, so we still
             *  need a read lock here. The number of times this happens and the
             *  relative cost of this operation should still be negligible,
             *  compared to other applications.
             */
            void
            wait_for_sort_tasks() {
                while (!tasks_.empty()) {
                    tasks_.pop().wait();
                }
            }

            template <class P, class I, class S, class Fun>
            void
            sort(P p, I first, S last, Fun f) {
                launch_sort_tasks(p, first, last, f);
                wait_for_sort_tasks();
            }

        private:
            detail::atomic_queue<basic_future<
                void,
                future_options<executor_opt<Executor>, continuable_opt>>>
                tasks_;
        };

        FUTURES_TEMPLATE(class I, class S, class Fun)
        (requires is_input_iterator_v<I> &&is_sentinel_for_v<S, I>
             &&is_indirectly_unary_invocable_v<Fun, I>
                 &&detail::is_copy_constructible_v<
                     Fun>) static FUTURES_CONSTANT_EVALUATED_CONSTEXPR
            void inline_for_each(I first, S last, Fun f) {
            for (; first != last; ++first) {
                f(*first);
            }
        }

        /// Complete overload of the for_each algorithm
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
         *  function template \c for_each
         */
        FUTURES_TEMPLATE(class E, class P, class I, class S, class Fun)
        (requires is_executor_v<E> &&is_partitioner_for_v<P, I, S>
             &&is_input_iterator_v<I> &&is_sentinel_for_v<S, I>
                 &&is_indirectly_unary_invocable_v<Fun, I>
                     &&detail::is_copy_constructible_v<Fun>)
            FUTURES_CONSTANT_EVALUATED_CONSTEXPR
            void run(E const &ex, P p, I first, S last, Fun f) const {
            FUTURES_IF_CONSTEXPR (
                detail::is_same_v<std::decay_t<E>, inline_executor>)
            {
                boost::ignore_unused(p);
                inline_for_each(first, last, f);
            } else {
                if (detail::is_constant_evaluated()) {
                    boost::ignore_unused(p);
                    inline_for_each(first, last, f);
                } else {
                    sort_graph<E>(ex).sort(p, first, last, f);
                }
            }
        }
    };

    /// Applies a function to a range of elements
    FUTURES_INLINE_VAR constexpr for_each_functor for_each;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_FOR_EACH_HPP
