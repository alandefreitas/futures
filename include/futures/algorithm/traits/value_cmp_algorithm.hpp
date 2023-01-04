//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_VALUE_CMP_ALGORITHM_HPP
#define FUTURES_ALGORITHM_TRAITS_VALUE_CMP_ALGORITHM_HPP

/**
 *  @file algorithm/traits/value_cmp_algorithm.hpp
 *  @brief `value_cmp_algorithm` trait
 *
 *  This file defines the `value_cmp_algorithm` trait representing a category
 *  of algorithms.
 *
 * The traits help us generate auxiliary algorithm overloads
 * This is somewhat similar to the pattern of traits and algorithms for ranges
 * and views It allows us to get algorithm overloads for free, including
 * default inference of the best execution policies
 *
 * @see
 * [`std::ranges::transform_view`](https://en.cppreference.com/w/cpp/ranges/transform_view)
 * @see [`std::ranges::view`](https://en.cppreference.com/w/cpp/ranges/view)
 */

#include <futures/algorithm/compare/equal_to.hpp>
#include <futures/algorithm/partitioner/halve_partitioner.hpp>
#include <futures/algorithm/partitioner/partitioner_for.hpp>
#include <futures/algorithm/policies.hpp>
#include <futures/algorithm/traits/is_indirectly_binary_invocable.hpp>
#include <futures/algorithm/traits/is_input_range.hpp>
#include <futures/algorithm/traits/iterator.hpp>
#include <futures/executor/default_executor.hpp>
#include <futures/executor/inline_executor.hpp>
#include <futures/algorithm/detail/execution.hpp>
#include <futures/algorithm/detail/make_policy_executor.hpp>

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

    /// Value-compare algorithm overloads
    /**
     * CRTP class with the overloads for classes that look for
     * elements in a sequence with an unary function.
     *
     * This includes algorithms such as @ref count and @ref find.
     */
    template <class Derived>
    class value_cmp_algorithm_functor {
    public:
        /// Execute the underlying algorithm
        /**
         *  \tparam E Executor type
         *  \tparam I Iterator type
         *  \tparam S Sentinel type
         *  \tparam P Partitioner type
         *  \tparam T Value to compare with the iterator value
         *  \param ex An executor instance
         *  \param p A partitioner instance
         *  \param first Iterator to first element in the range
         *  \param last Sentinel iterator to one element past the last
         *  \param value value to compare the elements to
         *  \return Result of the underlying algorithm
         */
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            class T>
        requires std::
            indirect_binary_predicate<std::ranges::equal_to, I, T const *>
#else
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            std::enable_if_t<
                is_executor_v<E> && is_partitioner_for_v<P, I, S>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_indirectly_binary_invocable_v<equal_to, T *, I>,
                int>
            = 0>
#endif
            FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
            operator()(E const &ex, P p, I first, S last, T const &value)
                const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(value));
            } else {
                return Derived().run(ex, p, first, last, std::move(value));
            }
        }

        /// Execute the algorithm with an execution policy
        /**
         * The execution policy is converted into the corresponding executor
         */
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            class T>
        requires std::
            indirect_binary_predicate<std::ranges::equal_to, I, T const *>
#else
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_partitioner_for_v<P, I, S> && is_input_iterator_v<I>
                    && is_sentinel_for_v<S, I>
                    && is_indirectly_binary_invocable_v<equal_to, T *, I>,
                int>
            = 0>
#endif
            FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
            operator()(E const &, P p, I first, S last, T const &value) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(value));
            } else {
                return Derived().run(
                    detail::make_policy_executor<E, I, S>(),
                    p,
                    first,
                    last,
                    std::move(value));
            }
        }

        /// Execute the underlying algorithm on a range of iterators
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::ranges::range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            class T>
        requires std::indirect_binary_predicate<
            std::ranges::equal_to,
            std::ranges::iterator_t<R>,
            T const *>
#else
        template <
            class E,
            class P,
            class R,
            class T,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
                    && is_partitioner_for_v<P, iterator_t<R>>
                    && is_input_range_v<R>
                    && is_indirectly_binary_invocable_v<
                        equal_to,
                        T *,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<T>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, P p, R &&r, T const &value) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(value));
            } else {
                return Derived()
                    .run(ex, p, std::begin(r), std::end(r), std::move(value));
            }
        }

        /// Execute the algorithm on a range of iterators and execution policy
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::ranges::range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            class T>
        requires std::indirect_binary_predicate<
            std::ranges::equal_to,
            std::ranges::iterator_t<R>,
            T const *>
#else
        template <
            class E,
            class P,
            class R,
            class T,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_partitioner_for_v<P, iterator_t<R>>
                    && is_input_range_v<R>
                    && is_indirectly_binary_invocable_v<
                        equal_to,
                        T *,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<T>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, P p, R &&r, T const &value) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(value));
            } else {
                return Derived()
                    .run(ex, p, std::begin(r), std::end(r), std::move(value));
            }
        }

        /// Execute the underlying algorithm with the default executor
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            class T>
        requires std::
            indirect_binary_predicate<std::ranges::equal_to, I, T const *>
#else
        template <
            class P,
            class I,
            class S,
            class T,
            std::enable_if_t<
                is_partitioner_for_v<P, I, S> && is_input_iterator_v<I>
                    && is_sentinel_for_v<S, I>
                    && is_indirectly_binary_invocable_v<equal_to, T *, I>
                    && detail::is_copy_constructible_v<T>,
                int>
            = 0>
#endif
            FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
            operator()(P p, I first, S last, T const &value) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(value));
            } else {
                return Derived().run(
                    make_default_executor(),
                    p,
                    first,
                    last,
                    std::move(value));
            }
        }

        /// Execute the algorithm on a range with the default executor
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::ranges::range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            class T>
        requires std::indirect_binary_predicate<
            std::ranges::equal_to,
            std::ranges::iterator_t<R>,
            T const *>
#else
        template <
            class P,
            class R,
            class T,
            std::enable_if_t<
                is_partitioner_for_v<P, iterator_t<R>> && is_input_range_v<R>
                    && is_indirectly_binary_invocable_v<
                        equal_to,
                        T *,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<T>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(P p, R &&r, T const &value) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(value));
            } else {
                return Derived().run(
                    make_default_executor(),
                    p,
                    std::begin(r),
                    std::end(r),
                    std::move(value));
            }
        }

        /// Execute the algorithm with the default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            class T>
        requires std::
            indirect_binary_predicate<std::ranges::equal_to, I, T const *>
#else
        template <
            class E,
            class I,
            class S,
            class T,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_indirectly_binary_invocable_v<equal_to, T *, I>,
                int>
            = 0>
#endif
            FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
            operator()(E const &ex, I first, S last, T const &value) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(value));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(first, last),
                    first,
                    last,
                    std::move(value));
            }
        }

        /// Execute the algorithm with execution policy and default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            class T>
        requires std::
            indirect_binary_predicate<std::ranges::equal_to, I, T const *>
#else
        template <
            class E,
            class I,
            class S,
            class T,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_indirectly_binary_invocable_v<equal_to, T *, I>,
                int>
            = 0>
#endif
            FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
            operator()(E const &ex, I first, S last, T const &value) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(value));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(first, last),
                    first,
                    last,
                    std::move(value));
            }
        }

        /// Execute algorithm on a range with the default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <executor E, std::ranges::range R, class T>
        requires std::indirect_binary_predicate<
            std::ranges::equal_to,
            std::ranges::iterator_t<R>,
            T const *>
#else
        template <
            class E,
            class R,
            class T,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
                    && is_input_range_v<R>
                    && is_indirectly_binary_invocable_v<
                        equal_to,
                        T *,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<T>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, R &&r, T const &value) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(value));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(std::forward<R>(r)),
                    std::begin(r),
                    std::end(r),
                    std::move(value));
            }
        }

        /// Execute algorithm on a range with policy and default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <execution_policy E, std::ranges::range R, class T>
        requires std::indirect_binary_predicate<
            std::ranges::equal_to,
            std::ranges::iterator_t<R>,
            T const *>
#else
        template <
            class E,
            class R,
            class T,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_input_range_v<R>
                    && is_indirectly_binary_invocable_v<
                        equal_to,
                        T *,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<T>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, R &&r, T const &value) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(value));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(std::forward<R>(r)),
                    std::begin(r),
                    std::end(r),
                    std::move(value));
            }
        }

        /// Execute algorithm with default partitioner and executor
#ifdef FUTURES_HAS_CONCEPTS
        template <std::input_iterator I, std::sentinel_for<I> S, class T>
        requires std::
            indirect_binary_predicate<std::ranges::equal_to, I, T const *>
#else
        template <
            class I,
            class S,
            class T,
            std::enable_if_t<
                is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_indirectly_binary_invocable_v<equal_to, T *, I>,
                int>
            = 0>
#endif
            FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
            operator()(I first, S last, T const &value) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(value));
            } else {
                return Derived().run(
                    make_default_executor(),
                    make_default_partitioner(first, last),
                    first,
                    last,
                    std::move(value));
            }
        }

        /// Execute algorithm on a range with default partitioner and executor
#ifdef FUTURES_HAS_CONCEPTS
        template <std::ranges::range R, class T>
        requires std::indirect_binary_predicate<
            std::ranges::equal_to,
            std::ranges::iterator_t<R>,
            T const *>
#else
        template <
            class R,
            class T,
            std::enable_if_t<
                is_input_range_v<R>
                    && is_indirectly_binary_invocable_v<
                        equal_to,
                        T *,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<T>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(R &&r, T const &value) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(value));
            } else {
                return Derived().run(
                    make_default_executor(),
                    make_default_partitioner(r),
                    std::begin(r),
                    std::end(r),
                    std::move(value));
            }
        }
    };
    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_VALUE_CMP_ALGORITHM_HPP
