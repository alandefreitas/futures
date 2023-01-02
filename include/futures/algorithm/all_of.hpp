//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_ALL_OF_HPP
#define FUTURES_ALGORITHM_ALL_OF_HPP

/**
 *  @file algorithm/all_of.hpp
 *  @brief `all_of` algorithm
 *
 *  This file defines the functor and callable for a parallel version of the
 *  `all_of` algorithm.
 */

#include <futures/future.hpp>
#include <futures/is_ready.hpp>
#include <futures/launch.hpp>
#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/traits/is_bidirectional_iterator.hpp>
#include <futures/algorithm/traits/is_derived_from.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/is_sentinel_for.hpp>
#include <futures/algorithm/traits/unary_invoke_algorithm.hpp>
#include <futures/detail/container/atomic_queue.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/algorithm/detail/execution.hpp>
#include <futures/algorithm/traits/detail/iter_concept.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <futures/detail/deps/boost/core/ignore_unused.hpp>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup functions Functions
     *  @{
     */

    /// Functor representing the overloads for the @ref all_of function
    class all_of_functor
        :
#ifdef FUTURES_DOXYGEN
        public unary_invoke_algorithm_functor
#else
        public unary_invoke_algorithm_functor<all_of_functor>
#endif
    {
        friend unary_invoke_algorithm_functor<all_of_functor>;

        template <class Executor>
        class all_of_graph : public boost::empty_value<Executor> {
        public:
            explicit all_of_graph(Executor const &ex)
                : boost::empty_value<Executor>(boost::empty_init, ex) {}

            // Each recursive call to this function can either calculate
            // `any_of` inline and push tasks to calculate the `any_of` for
            // some subranges. Returning `true` means we found a matching
            // element. Returning `false` means we didn't find a matching
            // element, but we might still find it in one of the parallel tasks
            // we have pushed.
            template <class P, class I, class S, class Fun>
            bool
            launch_all_of_tasks(P p, I first, S last, Fun f) {
                constexpr bool cannot_parallelize
                    = detail::is_same_v<Executor, inline_executor>
                      || !is_bidirectional_iterator_v<I>;
                if (cannot_parallelize) {
                    return std::all_of(first, last, f);
                }

                auto middle = p(first, last);
                bool const no_split = middle == last || middle == first;
                if (no_split) {
                    return std::all_of(first, last, f);
                }

                // Launch tasks for rhs: [middle, last]
                basic_future<
                    bool,
                    future_options<executor_opt<Executor>, continuable_opt>>
                    rhs_task = futures::async(
                        boost::empty_value<Executor>::get(),
                        [this, p, middle, last, f] {
                    return launch_all_of_tasks(p, middle, last, f);
                        });

                // Launch tasks for lhs: [first, middle]
                bool lhs_result = launch_all_of_tasks(p, first, middle, f);

                // When rhs is ready, probably because it had a small task
                // to solve, we check if any of the two subranges found a
                // matching element.
                if (is_ready(rhs_task)) {
                    return lhs_result && rhs_task.get();
                }

                // In the general case, we push rhs to the list of tasks we
                // have to await later
                tasks_.push(std::move(rhs_task));
                return lhs_result;
            }

            bool
            wait_for_all_of_tasks() {
                bool r = true;
                while (!tasks_.empty()) {
                    r = tasks_.pop().get() && r;
                }
                return r;
            }

            template <class P, class I, class S, class Fun>
            bool
            all_of(P p, I first, S last, Fun f) {
                bool partial = launch_all_of_tasks(p, first, last, f);
                return wait_for_all_of_tasks() && partial;
            }

        private:
            detail::atomic_queue<basic_future<
                bool,
                future_options<executor_opt<Executor>, continuable_opt>>>
                tasks_{};
        };

        FUTURES_TEMPLATE(class I, class S, class Fun)
        (requires is_input_iterator_v<I> &&is_sentinel_for_v<S, I>
             &&is_indirectly_unary_invocable_v<Fun, I>
                 &&detail::is_copy_constructible_v<
                     Fun>) static FUTURES_CONSTANT_EVALUATED_CONSTEXPR
            bool inline_all_of(I first, S last, Fun p) {
            for (; first != last; ++first) {
                if (!p(*first)) {
                    return false;
                }
            }
            return true;
        }

        /// Complete overload of the all_of algorithm
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
         *  function template \c all_of
         */
        FUTURES_TEMPLATE(class E, class P, class I, class S, class Fun)
        (requires is_executor_v<E> &&is_partitioner_for_v<P, I, S>
             &&is_input_iterator_v<I> &&is_sentinel_for_v<S, I>
                 &&is_indirectly_unary_invocable_v<Fun, I>
                     &&detail::is_copy_constructible_v<Fun>)
            FUTURES_CONSTANT_EVALUATED_CONSTEXPR
            bool run(E const &ex, P p, I first, S last, Fun f) const {
            FUTURES_IF_CONSTEXPR (
                std::is_same<std::decay_t<E>, inline_executor>::value)
            {
                boost::ignore_unused(p, ex);
                return inline_all_of(first, last, f);
            } else {
                if (detail::is_constant_evaluated()) {
                    boost::ignore_unused(p);
                    return inline_all_of(first, last, f);
                } else {
                    return all_of_graph<E>(ex).all_of(p, first, last, f);
                }
            }
        }
    };

    /// Checks if a predicate is true for all the elements in a range
    FUTURES_INLINE_VAR constexpr all_of_functor all_of;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_ALL_OF_HPP
