//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_FIND_IF_HPP
#define FUTURES_ALGORITHM_FIND_IF_HPP

/**
 *  @file algorithm/find_if.hpp
 *  @brief `find_if` algorithm
 *
 *  This file defines the functor and callable for a parallel version of the
 *  `find_if` algorithm.
 */

#include <futures/future.hpp>
#include <futures/is_ready.hpp>
#include <futures/launch.hpp>
#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/traits/is_bidirectional_iterator.hpp>
#include <futures/algorithm/traits/iter_difference.hpp>
#include <futures/algorithm/traits/unary_invoke_algorithm.hpp>
#include <futures/detail/container/atomic_queue.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/algorithm/detail/execution.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <futures/detail/deps/boost/core/ignore_unused.hpp>
#include <bitset>


namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup functions Functions
     *  @{
     */

    class find_functor;
    class find_if_not_functor;

    /// Functor representing the overloads for the @ref find_if function
    class find_if_functor
        :
#ifdef FUTURES_DOXYGEN
        public unary_invoke_algorithm_functor
#else
        public unary_invoke_algorithm_functor<find_if_functor>
#endif
    {
        friend unary_invoke_algorithm_functor<find_if_functor>;
        friend find_functor;
        friend find_if_not_functor;

        template <class Executor, class I>
        class find_if_graph : public boost::empty_value<Executor> {
        public:
            explicit find_if_graph(Executor const &ex)
                : boost::empty_value<Executor>(boost::empty_init, ex) {}

            template <class P, class S, class Fun>
            std::pair<I, std::size_t>
            launch_find_if_tasks(
                P p,
                I first,
                S last,
                S overall_last,
                Fun f,
                std::size_t level = 0,
                std::size_t branch = 0) {
                auto middle = p(first, last);
                bool const no_split = middle == last || middle == first;
                constexpr bool cannot_parallelize =
                    // std::is_same_v<Executor, inline_executor>
                    //||
                    !is_bidirectional_iterator_v<I>;
                if (no_split || cannot_parallelize) {
                    // run sequential algorithm
                    I subrange_it = std::find_if(first, last, f);
                    if (subrange_it == last) {
                        // overall last indicates nothing has been found
                        return std::make_pair(overall_last, branch);
                    } else {
                        // subrange it indicates something has been found
                        return std::make_pair(subrange_it, branch);
                    }
                }

                // Create task that launches tasks for rhs: [middle, last]
                std::size_t rhs_branch = branch;
                rhs_branch |= std::size_t(1)
                              << (8 * sizeof(std::size_t) - 1 - level);
                basic_future<
                    std::pair<I, std::size_t>,
                    future_options<executor_opt<Executor>, continuable_opt>>
                    rhs_task = futures::async(
                        boost::empty_value<Executor>::get(),
                        [this,
                         p,
                         middle,
                         last,
                         overall_last,
                         f,
                         level,
                         rhs_branch] {
                    return launch_find_if_tasks(
                        p,
                        middle,
                        last,
                        overall_last,
                        f,
                        level + 1,
                        rhs_branch);
                        });

                // Launch tasks for lhs: [first, middle]
                branch &= ~(
                    std::size_t(1) << (8 * sizeof(std::size_t) - 1 - level));
                std::pair<I, std::size_t> lhs_result = launch_find_if_tasks(
                    p,
                    first,
                    middle,
                    overall_last,
                    f,
                    level + 1,
                    branch);

                // When rhs is ready, probably because it had a small task
                // to solve, we check if any of the two sub-ranges found a
                // matching element.
                if (is_ready(rhs_task)) {
                    // We didn't find it in lhs
                    if (lhs_result.first == overall_last) {
                        return rhs_task.get();
                    }
                    return lhs_result;
                }

                // In the general case, we push rhs to the list of tasks we
                // have to await later
                tasks_.push(std::move(rhs_task));
                return lhs_result;
            }

            template <class S>
            std::pair<I, std::size_t>
            wait_for_find_if_tasks(S last) {
                std::pair<I, std::size_t> min_it{ last, -1 };
                while (!tasks_.empty()) {
                    std::pair<I, std::size_t> r = tasks_.pop().get();
                    if (r.first != last && r.second < min_it.second) {
                        min_it = r;
                    }
                }
                return min_it;
            }

            template <class P, class S, class Fun>
            I
            find_if(P p, I first, S last, Fun f) {
                // inline sub-ranges result
                std::pair<I, std::size_t>
                    partial = launch_find_if_tasks(p, first, last, last, f);
                // async sub-ranges result
                std::pair<I, std::size_t> partial_2 = wait_for_find_if_tasks(
                    last);
                if (partial.first == last) {
                    return partial_2.first;
                }
                if (partial_2.first == last) {
                    return partial.first;
                }
                if (partial.second < partial_2.second) {
                    return partial.first;
                }
                return partial_2.first;
            }

        private:
            detail::atomic_queue<basic_future<
                std::pair<I, std::size_t>,
                future_options<executor_opt<Executor>, continuable_opt>>>
                tasks_{};
        };

        FUTURES_TEMPLATE(class I, class S, class Fun)
        (requires is_input_iterator_v<I> &&is_sentinel_for_v<S, I>
             &&is_indirectly_unary_invocable_v<Fun, I>
                 &&detail::is_copy_constructible_v<
                     Fun>) static FUTURES_CONSTANT_EVALUATED_CONSTEXPR I
            inline_find_if(I first, S last, Fun p) {
            for (; first != last; ++first) {
                if (p(*first)) {
                    return first;
                }
            }
            return last;
        }

        /// Complete overload of the find_if algorithm
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
         *  function template \c find_if
         */
        FUTURES_TEMPLATE(class E, class P, class I, class S, class Fun)
        (requires is_executor_v<E> &&is_partitioner_v<P, I, S>
             &&is_input_iterator_v<I> &&is_sentinel_for_v<S, I>
                 &&is_indirectly_unary_invocable_v<Fun, I>
                     &&detail::is_copy_constructible_v<Fun>)
            FUTURES_CONSTANT_EVALUATED_CONSTEXPR I
            run(E const &ex, P p, I first, S last, Fun f) const {
            FUTURES_IF_CONSTEXPR (
                detail::is_same_v<std::decay_t<E>, inline_executor>)
            {
                boost::ignore_unused(p);
                return inline_find_if(first, last, f);
            } else {
                if (detail::is_constant_evaluated()) {
                    boost::ignore_unused(p);
                    return inline_find_if(first, last, f);
                } else {
                    return find_if_graph<E, I>(ex).find_if(p, first, last, f);
                }
            }
        }
    };

    /// Finds the first element satisfying specific criteria
    FUTURES_INLINE_VAR constexpr find_if_functor find_if;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_FIND_IF_HPP
