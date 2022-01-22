//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FOR_EACH_H
#define FUTURES_FOR_EACH_H

#include <futures/algorithm/partitioner/partitioner.h>
#include <futures/algorithm/traits/algorithm_traits.h>
#include <futures/futures.h>
#include <futures/algorithm/detail/traits/range/range/concepts.h>
#include <futures/algorithm/detail/try_async.h>
#include <futures/futures/detail/empty_base.h>
#include <execution>
#include <variant>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref for_each function
    class for_each_functor
        : public detail::unary_invoke_algorithm_functor<for_each_functor>
    {
    public:
        // Let only unary_invoke_algorithm_functor access the primary sort
        // function template
        friend detail::unary_invoke_algorithm_functor<for_each_functor>;

    private:
        /// \brief Internal class that takes care of the sorting tasks and its
        /// incomplete tasks
        ///
        /// If we could make sure no executors would ever block, recursion
        /// wouldn't be a problem, and we wouldn't need this class. In fact,
        /// this is what most related libraries do, counting on the executor to
        /// be some kind of work stealing thread pool.
        ///
        /// However, we cannot count on that, or these algorithms wouldn't work
        /// for many executors in which we are interested, such as an io_context
        /// or a thread pool that doesn't steel work (like asio's). So we need
        /// to separate the process of launching the tasks from the process of
        /// waiting for them. Fortunately, we can count that most executors
        /// wouldn't need this blocking procedure very often, because that's
        /// what usually make them useful executors. We also assume that, unlike
        /// in the other applications, the cost of this reading lock is trivial
        /// compared to the cost of the whole procedure.
        ///
        template <class Executor>
        class sorter : public detail::maybe_empty<Executor>
        {
        public:
            explicit sorter(const Executor &ex)
                : detail::maybe_empty<Executor>(ex) {}

            /// \brief Get executor from the base class as a function for
            /// convenience
            const Executor &
            ex() const {
                return detail::maybe_empty<Executor>::get();
            }

            /// \brief Get executor from the base class as a function for
            /// convenience
            Executor &
            ex() {
                return detail::maybe_empty<Executor>::get();
            }

            template <class P, class I, class S, class Fun>
            void
            launch_sort_tasks(P p, I first, S last, Fun f) {
                auto middle = p(first, last);
                const bool too_small = middle == last;
                constexpr bool cannot_parallelize
                    = std::is_same_v<
                          Executor,
                          inline_executor> || futures::detail::forward_iterator<I>;
                if (too_small || cannot_parallelize) {
                    std::for_each(first, last, f);
                } else {
                    // Run for_each on rhs: [middle, last]
                    cfuture<void> rhs_task = futures::
                        async(ex(), [this, p, middle, last, f] {
                            launch_sort_tasks(p, middle, last, f);
                        });

                    // Run for_each on lhs: [first, middle]
                    launch_sort_tasks(p, first, middle, f);

                    // When lhs is ready, we check on rhs
                    if (!is_ready(rhs_task)) {
                        // Put rhs_task on the list of tasks we need to await
                        // later This ensures we only deal with the task queue
                        // if we really need to
                        std::unique_lock write_lock(tasks_mutex_);
                        tasks_.emplace_back(std::move(rhs_task));
                    }
                }
            }

            /// \brief Wait for all tasks to finish
            ///
            /// This might sound like it should be as simple as a
            /// when_all(tasks_). However, while we wait for some tasks here,
            /// the running tasks might be enqueuing more tasks, so we still
            /// need a read lock here. The number of times this happens and the
            /// relative cost of this operation should still be negligible,
            /// compared to other applications.
            ///
            /// \return `true` if we had to wait for any tasks
            bool
            wait_for_sort_tasks() {
                tasks_mutex_.lock_shared();
                bool waited_any = false;
                while (!tasks_.empty()) {
                    tasks_mutex_.unlock_shared();
                    tasks_mutex_.lock();
                    detail::small_vector<futures::cfuture<void>> stolen_tasks(
                        std::make_move_iterator(tasks_.begin()),
                        std::make_move_iterator(tasks_.end()));
                    tasks_.clear();
                    tasks_mutex_.unlock();
                    when_all(stolen_tasks).wait();
                    waited_any = true;
                }
                return waited_any;
            }

            template <class P, class I, class S, class Fun>
            void
            sort(P p, I first, S last, Fun f) {
                launch_sort_tasks(p, first, last, f);
                wait_for_sort_tasks();
            }

        private:
            detail::small_vector<futures::cfuture<void>> tasks_;
            std::shared_mutex tasks_mutex_;
        };

        /// \brief Complete overload of the for_each algorithm
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
        /// \brief function template \c for_each
        template <
            class FullAsync = std::false_type,
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                is_executor_v<
                    E> && is_partitioner_v<P, I, S> && is_input_iterator_v<I> && futures::detail::sentinel_for<S, I> && futures::detail::indirectly_unary_invocable<Fun, I> && std::is_copy_constructible_v<Fun>,
                int> = 0
#endif
            >
        auto
        run(const E &ex, P p, I first, S last, Fun f) const {
            if constexpr (FullAsync::value) {
                // If full async, launching the tasks and solving small tasks
                // also happen asynchronously
                return async(ex, [ex, p, first, last, f]() {
                    sorter<E>(ex).sort(p, first, last, f);
                });
            } else {
                // Else, we try to solve small tasks and launching other tasks
                // if it's worth splitting the problem
                sorter<E>(ex).sort(p, first, last, f);
            }
        }
    };

    /// \brief Applies a function to a range of elements
    inline constexpr for_each_functor for_each;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_FOR_EACH_H
