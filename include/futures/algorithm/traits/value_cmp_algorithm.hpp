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
 * @see https://en.cppreference.com/w/cpp/ranges/transform_view
 * @see https://en.cppreference.com/w/cpp/ranges/view
 */

#include <futures/algorithm/compare/equal_to.hpp>
#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/policies.hpp>
#include <futures/algorithm/traits/is_indirectly_binary_invocable.hpp>
#include <futures/executor/default_executor.hpp>
#include <futures/executor/inline_executor.hpp>
#include <futures/algorithm/detail/execution.hpp>

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
     * elements in a sequence with an unary function This includes
     * algorithms such as for_each, any_of, all_of, ...
     */
    template <class Derived>
    class value_cmp_algorithm_functor {
    public:
        /// Complete overload
        template <
            class E,
            class P,
            class I,
            class S,
            class T FUTURES_REQUIRE(
                (is_executor_v<E> && is_partitioner_v<P, I, S>
                 && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                 && is_indirectly_binary_invocable_v<equal_to, T *, I>) )>
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR
        FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, P p, I first, S last, T f) const {
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

        /// Execution policy instead of executor
        template <
            class E,
            class P,
            class I,
            class S,
            class T FUTURES_REQUIRE((
                !is_executor_v<E> && is_execution_policy_v<E>
                && is_partitioner_v<P, I, S> && is_input_iterator_v<I>
                && is_sentinel_for_v<S, I>
                && is_indirectly_binary_invocable_v<equal_to, T *, I>) )>
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR
        FUTURES_DETAIL(decltype(auto))
        operator()(E const &, P p, I first, S last, T f) const {
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

        /// Overload for Ranges
        template <
            class E,
            class P,
            class R,
            class T FUTURES_REQUIRE((
                (is_executor_v<E>
                 || is_execution_policy_v<E>) &&is_range_partitioner_v<P, R>
                && is_input_range_v<R>
                && is_indirectly_binary_invocable_v<equal_to, T *, iterator_t<R>>
                && std::is_copy_constructible_v<T>) )>
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR
        FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, P p, R &&r, T f) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            } else {
                return Derived()
                    .run(ex, p, std::begin(r), std::end(r), std::move(f));
            }
        }

        /// Overload for Iterators / default parallel executor
        template <
            class P,
            class I,
            class S,
            class T FUTURES_REQUIRE((
                is_partitioner_v<P, I, S> && is_input_iterator_v<I>
                && is_sentinel_for_v<S, I>
                && is_indirectly_binary_invocable_v<equal_to, T *, I>
                && std::is_copy_constructible_v<T>) )>
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR
        FUTURES_DETAIL(decltype(auto))
        operator()(P p, I first, S last, T f) const {
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

        /// Overload for Ranges / default parallel executor
        template <
            class P,
            class R,
            class T FUTURES_REQUIRE((
                is_range_partitioner_v<P, R> && is_input_range_v<R>
                && is_indirectly_binary_invocable_v<equal_to, T *, iterator_t<R>>
                && std::is_copy_constructible_v<T>) )>
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR
        FUTURES_DETAIL(decltype(auto))
        operator()(P p, R &&r, T f) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            } else {
                return Derived().run(
                    make_default_executor(),
                    p,
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            }
        }

        /// Overload for Iterators / default partitioner
        template <
            class E,
            class I,
            class S,
            class T FUTURES_REQUIRE((
                (is_executor_v<E>
                 || is_execution_policy_v<E>) &&is_input_iterator_v<I>
                && is_sentinel_for_v<S, I>
                && is_indirectly_binary_invocable_v<equal_to, T *, I>) )>
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR
        FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, I first, S last, T f) const {
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

        /// Overload for Ranges / default partitioner
        template <
            class E,
            class R,
            class T FUTURES_REQUIRE((
                (is_executor_v<E>
                 || is_execution_policy_v<E>) &&is_input_range_v<R>
                && is_indirectly_binary_invocable_v<equal_to, T *, iterator_t<R>>
                && std::is_copy_constructible_v<T>) )>
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR
        FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, R &&r, T f) const {
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

        /// Overload for Iterators / default executor / default partitioner
        template <
            class I,
            class S,
            class T FUTURES_REQUIRE(
                (is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                 && is_indirectly_binary_invocable_v<equal_to, T *, I>) )>
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR
        FUTURES_DETAIL(decltype(auto))
        operator()(I first, S last, T f) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(f));
            } else {
                return Derived().run(
                    make_default_executor(),
                    make_default_partitioner(first, last),
                    first,
                    last,
                    std::move(f));
            }
        }

        /// Overload for Ranges / default executor / default partitioner
        template <
            class R,
            class T FUTURES_REQUIRE((
                is_input_range_v<R>
                && is_indirectly_binary_invocable_v<equal_to, T *, iterator_t<R>>
                && std::is_copy_constructible_v<T>) )>
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR
        FUTURES_DETAIL(decltype(auto))
        operator()(R &&r, T f) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            } else {
                return Derived().run(
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

#endif // FUTURES_ALGORITHM_TRAITS_VALUE_CMP_ALGORITHM_HPP
