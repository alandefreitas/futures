//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_UNARY_INVOKE_ALGORITHM_HPP
#define FUTURES_ALGORITHM_TRAITS_UNARY_INVOKE_ALGORITHM_HPP

/**
 *  @file algorithm/traits/unary_invoke_algorithm.hpp
 *  @brief `unary_invoke_algorithm` trait
 *
 *  This file defines the `unary_invoke_algorithm` trait.
 *
 *  The traits help us generate auxiliary algorithm overloads
 *  This is somewhat similar to the pattern of traits and algorithms for ranges
 *  and views It allows us to get algorithm overloads for free, including
 *  default inference of the best execution policies
 *
 *  @see [`std::ranges::transform_view`](https://en.cppreference.com/w/cpp/ranges/transform_view)
 *  @see [`std::ranges::view`](https://en.cppreference.com/w/cpp/ranges/view)
 */

#include <futures/algorithm/partitioner/default_partitioner.hpp>
#include <futures/algorithm/partitioner/halve_partitioner.hpp>
#include <futures/algorithm/partitioner/partitioner_for.hpp>
#include <futures/algorithm/policies.hpp>
#include <futures/algorithm/traits/is_indirectly_unary_invocable.hpp>
#include <futures/algorithm/traits/is_input_range.hpp>
#include <futures/executor/default_executor.hpp>
#include <futures/executor/inline_executor.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/algorithm/detail/execution.hpp>

#ifdef FUTURES_HAS_CONCEPTS
#    include <ranges>
#endif

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup algorithm-traits Algorithm Traits
     *  @{
     */

    /// Overloads for unary invoke algorithms
    /**
     * CRTP class with the overloads for algorithm functors that iterate
     * elements in a sequence with an unary function.
     *
     * This includes algorithms such as @ref for_each and @ref any_of.
     */
    template <class Derived>
    class unary_invoke_algorithm_functor {
    public:
        /// Execute the underlying algorithm
        /**
         *  \tparam E Executor type
         *  \tparam I Iterator type
         *  \tparam S Sentinel type
         *  \tparam P Partitioner type
         *  \tparam Fun Function type
         *  \param ex An executor instance
         *  \param p A partitioner instance
         *  \param first Iterator to first element in the range
         *  \param last Sentinel iterator to one element past the last
         *  \param f Function invocable with the return type of the iterator
         *  \return Result of the underlying algorithm
         */
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            std::indirectly_unary_invocable<I> Fun>
#else
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun,
            std::enable_if_t<
                is_executor_v<E> && is_partitioner_for_v<P, I, S>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_indirectly_unary_invocable_v<Fun, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(FUTURES_DETAIL(decltype(auto)))
        operator()(E const &ex, P p, I first, S last, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(f));
            } else {
                return Derived().run(ex, p, first, last, std::move(f));
            }
        }


        /// Execute the underlying algorithm with an execution policy
        /**
         * The execution policy is converted into the corresponding executor
         */
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            std::indirectly_unary_invocable<I> Fun>
#else
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_partitioner_for_v<P, I, S> && is_input_iterator_v<I>
                    && is_sentinel_for_v<S, I>
                    && is_indirectly_unary_invocable_v<Fun, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &, P p, I first, S last, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(f));
            } else {
                return Derived().run(
                    make_policy_executor<E, I, S>(),
                    p,
                    first,
                    last,
                    std::move(f));
            }
        }

        /// Execute the algorithm with a range of iterators
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::ranges::input_range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            std::indirectly_unary_invocable<std::ranges::iterator_t<R>> Fun>
#else
        template <
            class E,
            class P,
            class R,
            class Fun,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
                    && is_partitioner_for_v<P, iterator_t<R>>
                    && is_input_range_v<R>
                    && is_indirectly_unary_invocable_v<Fun, iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, P p, R &&r, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            } else {
                return
                operator()(ex, p, std::begin(r), std::end(r), std::move(f));
            }
        }

        /// Execute the algorithm with a range of iterators and execution policy
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::ranges::range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            std::indirectly_unary_invocable<std::ranges::iterator_t<R>> Fun>
#else
        template <
            class E,
            class P,
            class R,
            class Fun,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_partitioner_for_v<P, iterator_t<R>>
                    && is_input_range_v<R>
                    && is_indirectly_unary_invocable_v<Fun, iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, P p, R &&r, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            } else {
                return
                operator()(ex, p, std::begin(r), std::end(r), std::move(f));
            }
        }

        /// Execute the underlying algorithm with a default executor
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            std::indirectly_unary_invocable<I> Fun>
#else
        template <
            class P,
            class I,
            class S,
            class Fun,
            std::enable_if_t<
                is_partitioner_for_v<P, I, S> && is_input_iterator_v<I>
                    && is_sentinel_for_v<S, I>
                    && is_indirectly_unary_invocable_v<Fun, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(P p, I first, S last, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(f));
            } else {
                return Derived()
                    .run(make_default_executor(), p, first, last, std::move(f));
            }
        }

        /// Execute the algorithm on a range with a default executor
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::ranges::range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            std::indirectly_unary_invocable<std::ranges::iterator_t<R>> Fun>
#else
        template <
            class P,
            class R,
            class Fun,
            std::enable_if_t<
                is_partitioner_for_v<P, iterator_t<R>> && is_input_range_v<R>
                    && is_indirectly_unary_invocable_v<Fun, iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(P p, R &&r, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            } else {
                return operator()(
                    make_default_executor(),
                    p,
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            }
        }

        /// Execute the underlying algorithm with a default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            std::indirectly_unary_invocable<I> Fun>
#else
        template <
            class E,
            class I,
            class S,
            class Fun,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_indirectly_unary_invocable_v<Fun, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, I first, S last, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(first, last),
                    first,
                    last,
                    std::move(f));
            }
        }

        /// Execute the algorithm with execution policy and default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            std::indirectly_unary_invocable<I> Fun>
#else
        template <
            class E,
            class I,
            class S,
            class Fun,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_indirectly_unary_invocable_v<Fun, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, I first, S last, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(first, last),
                    first,
                    last,
                    std::move(f));
            }
        }

        /// Execute the algorithm on a range with the default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::ranges::input_range R,
            std::indirectly_unary_invocable<std::ranges::iterator_t<R>> Fun>
#else
        template <
            class E,
            class R,
            class Fun,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
                    && is_input_range_v<R>
                    && is_indirectly_unary_invocable_v<Fun, iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, R &&r, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(std::forward<R>(r)),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            }
        }

        /// Execute the algorithm on a range with policy and default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::ranges::input_range R,
            std::indirectly_unary_invocable<std::ranges::iterator_t<R>> Fun>
#else
        template <
            class E,
            class R,
            class Fun,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_input_range_v<R>
                    && is_indirectly_unary_invocable_v<Fun, iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, R &&r, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(std::forward<R>(r)),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            }
        }

        /// Execute the algorithm with default executor and partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::input_iterator I,
            std::sentinel_for<I> S,
            std::indirectly_unary_invocable<I> Fun>
#else
        template <
            class I,
            class S,
            class Fun,
            std::enable_if_t<
                is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_indirectly_unary_invocable_v<Fun, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(I first, S last, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(f));
            } else {
                return operator()(
                    make_default_executor(),
                    make_default_partitioner(first, last),
                    first,
                    last,
                    std::move(f));
            }
        }

        /// Execute algorithm on a range with default executor and partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::ranges::input_range R,
            std::indirectly_unary_invocable<std::ranges::iterator_t<R>> Fun>
#else
        template <
            class R,
            class Fun,
            std::enable_if_t<
                is_input_range_v<R>
                    && is_indirectly_unary_invocable_v<Fun, iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(R &&r, Fun f) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(make_grain_size(r.size())),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            } else {
                return operator()(
                    make_default_executor(),
                    make_default_partitioner(r),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            }
        }
    };

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_UNARY_INVOKE_ALGORITHM_HPP
