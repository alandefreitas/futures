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
#include <futures/algorithm/traits/is_forward_iterator.hpp>
#include <futures/algorithm/traits/iter_difference.hpp>
#include <futures/algorithm/traits/unary_invoke_algorithm.hpp>
#include <futures/detail/container/atomic_queue.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <bitset>
#include <execution>
#include <variant>

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
        : public unary_invoke_algorithm_functor<find_if_functor> {
        friend unary_invoke_algorithm_functor<find_if_functor>;
        friend find_functor;
        friend find_if_not_functor;

        template <class Executor, class I>
        class find_if_graph : public boost::empty_value<Executor> {
        public:
            explicit find_if_graph(Executor const &ex)
                : boost::empty_value<Executor>(boost::empty_init, ex) {}

            template <class P, class S, class Fun>
            std::pair<I, std::bitset<64>>
            launch_find_if_tasks(
                P p,
                I first,
                S last,
                S overall_last,
                Fun f,
                std::size_t level = 0,
                std::bitset<64> branch = 0) {
                auto middle = p(first, last);
                iter_difference_t<I> const too_small = middle == last;
                constexpr iter_difference_t<I> cannot_parallelize
                    = std::is_same_v<Executor, inline_executor>
                      || is_forward_iterator_v<I>;
                if (too_small || cannot_parallelize) {
                    I subrange_it = std::find_if(first, last, f);
                    if (subrange_it == last) {
                        return std::make_pair(overall_last, branch);
                    } else {
                        return std::make_pair(subrange_it, branch);
                    }
                } else {
                    // Create task that launches tasks for rhs: [middle, last]
                    std::bitset<64> rhs_branch = branch;
                    rhs_branch[64 - level - 1] = true;
                    basic_future<
                        std::pair<I, std::bitset<64>>,
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
                    branch[64 - level - 1] = false;
                    std::pair<I, std::bitset<64>>
                        lhs_result = launch_find_if_tasks(
                            p,
                            first,
                            middle,
                            overall_last,
                            f,
                            level + 1,
                            branch);

                    // When lhs is ready, we check on rhs
                    if (!is_ready(rhs_task)) {
                        // Put rhs_task on the list of tasks we need to await
                        // later. This ensures we only deal with the task queue
                        // if we really need to.
                        if (lhs_result.first != overall_last) {
                            rhs_task.detach();
                        } else {
                            tasks_.push(std::move(rhs_task));
                        }
                        return lhs_result;
                    } else {
                        if (lhs_result.first != overall_last) {
                            return lhs_result;
                        } else {
                            return rhs_task.get();
                        }
                    }
                }
            }

            template <class S>
            I
            wait_for_find_if_tasks(S last) {
                std::pair<I, std::bitset<64>> min_it{ last, -1 };
                while (!tasks_.empty()) {
                    std::pair<I, std::bitset<64>> r = tasks_.pop().get();
                    if (r.first != last
                        && r.second.count() < min_it.second.count())
                    {
                        min_it = r;
                    }
                }
                return min_it.first;
            }

            template <class P, class S, class Fun>
            I
            find_if(P p, I first, S last, Fun f) {
                std::pair<I, std::bitset<64>>
                    partial = launch_find_if_tasks(p, first, last, last, f);
                if (partial.first != last) {
                    return partial.first;
                } else {
                    return wait_for_find_if_tasks(last);
                }
            }

        private:
            detail::atomic_queue<basic_future<
                std::pair<I, std::bitset<64>>,
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
        static FUTURES_CONSTANT_EVALUATED_CONSTEXPR I
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
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR I
        run(E const &ex, P p, I first, S last, Fun f) const {
            if constexpr (std::is_same_v<std::decay_t<E>, inline_executor>) {
                return inline_find_if(first, last, f);
            } else {
                if (detail::is_constant_evaluated()) {
                    return inline_find_if(first, last, f);
                } else {
                    return find_if_graph<E, I>(ex).find_if(p, first, last, f);
                }
            }
        }
    };

    /// Finds the first element satisfying specific criteria
    inline constexpr find_if_functor find_if;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_FIND_IF_HPP
