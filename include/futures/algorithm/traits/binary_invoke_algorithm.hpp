//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_BINARY_INVOKE_ALGORITHM_HPP
#define FUTURES_ALGORITHM_TRAITS_BINARY_INVOKE_ALGORITHM_HPP

/**
 *  @file algorithm/traits/binary_invoke_algorithm.hpp
 *  @brief `binary_invoke_algorithm` trait
 *
 *  This file defines the `binary_invoke_algorithm` trait.
 */

/**
 *  @file algorithm/traits/binary_invoke_algorithm.hpp
 *  @brief `binary_invoke_algorithm` trait
 *
 *  This file defines the `binary_invoke_algorithm` trait.
 */

#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/policies.hpp>
#include <futures/algorithm/traits/is_indirectly_binary_invocable.hpp>
#include <futures/algorithm/traits/is_input_range.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/algorithm/detail/execution.hpp>
#include <numeric>


namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup algorithm-traits Algorithm Traits
     *  @{
     */

    /// Binary algorithm overloads
    /**
     * CRTP class with the overloads for algorithms that aggregate
     * elements in a sequence with an binary function.
     *
     * This includes algorithms such as reduce and accumulate.
     */
    template <class Derived>
    class binary_invoke_algorithm_functor {
    public:
        /// Complete overload
#ifdef FUTURES_HAS_CONCEPTS
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>>
        requires is_executor_v<E> && is_partitioner_v<P, I, S>
                 && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                 && detail::is_same_v<iter_value_t<I>, T>
                 && is_indirectly_binary_invocable_v<Fun, I, I>
                 && detail::is_copy_constructible_v<Fun>
#else
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_executor_v<E> && is_partitioner_v<P, I, S>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && detail::is_same_v<iter_value_t<I>, T>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, P p, I first, S last, T i, Fun f = std::plus<>())
            const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    i,
                    std::move(f));
            } else {
                return Derived().run(ex, p, first, last, i, std::move(f));
            }
        }

        /// Overload for default init value
#ifdef FUTURES_HAS_CONCEPTS
        template <class E, class P, class I, class S, class Fun = std::plus<>>
        requires is_executor_v<E> && is_partitioner_v<P, I, S>
                 && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                 && is_indirectly_binary_invocable_v<Fun, I, I>
                 && detail::is_copy_constructible_v<Fun>
#else
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_executor_v<E> && is_partitioner_v<P, I, S>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, P p, I first, S last, Fun f = std::plus<>())
            const {
            if (first == last) {
                return iter_value_t<I>{};
            }

            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::next(first),
                    last,
                    *first,
                    std::move(f));
            } else {
                return Derived()
                    .run(ex, p, std::next(first), last, *first, std::move(f));
            }
        }

        /// Overload for execution policy instead of executor
#ifdef FUTURES_HAS_CONCEPTS
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>>
        requires(
            !is_executor_v<E> && is_execution_policy_v<E>
            && is_partitioner_v<P, I, S> && is_input_iterator_v<I>
            && is_sentinel_for_v<S, I> && detail::is_same_v<iter_value_t<I>, T>
            && is_indirectly_binary_invocable_v<Fun, I, I>
            && detail::is_copy_constructible_v<Fun>)
#else
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_partitioner_v<P, I, S> && is_input_iterator_v<I>
                    && is_sentinel_for_v<S, I>
                    && detail::is_same_v<iter_value_t<I>, T>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &, P p, I first, S last, T i, Fun f = std::plus<>())
            const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    i,
                    std::move(f));
            } else {
                return operator()(
                    make_policy_executor<E, I, S>(),
                    p,
                    first,
                    last,
                    i,
                    std::move(f));
            }
        }

        /// Overload for execution policy instead of executor / default init
        /// value
#ifdef FUTURES_HAS_CONCEPTS
        template <class E, class P, class I, class S, class Fun = std::plus<>>
        requires(
            !is_executor_v<E> && is_execution_policy_v<E>
            && is_partitioner_v<P, I, S> && is_input_iterator_v<I>
            && is_sentinel_for_v<S, I>
            && is_indirectly_binary_invocable_v<Fun, I, I>
            && detail::is_copy_constructible_v<Fun>)
#else
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun = std::plus<>,
            std::enable_if_t<
                (!is_executor_v<E> && is_execution_policy_v<E>
                 && is_partitioner_v<P, I, S> && is_input_iterator_v<I>
                 && is_sentinel_for_v<S, I>
                 && is_indirectly_binary_invocable_v<Fun, I, I>
                 && detail::is_copy_constructible_v<Fun>),
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &, P p, I first, S last, Fun f = std::plus<>())
            const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    std::move(f));
            } else {
                return operator()(
                    make_policy_executor<E, I, S>(),
                    p,
                    first,
                    last,
                    std::move(f));
            }
        }

        /// Overload for Ranges
#ifdef FUTURES_HAS_CONCEPTS
        template <class E, class P, class R, class T, class Fun = std::plus<>>
        requires(is_executor_v<E> || is_execution_policy_v<E>)
                && is_range_partitioner_v<P, R> && is_input_range_v<R>
                && detail::is_same_v<range_value_t<R>, T>
                && is_indirectly_binary_invocable_v<
                    Fun,
                    iterator_t<R>,
                    iterator_t<R>>
                && detail::is_copy_constructible_v<Fun>
#else
        template <
            class E,
            class P,
            class R,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                (is_executor_v<E>
                 || is_execution_policy_v<E>) &&is_range_partitioner_v<P, R>
                    && is_input_range_v<R>
                    && detail::is_same_v<range_value_t<R>, T>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, P p, R &&r, T i, Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    i,
                    std::move(f));
            } else {
                return
                operator()(ex, p, std::begin(r), std::end(r), i, std::move(f));
            }
        }

        /// Overload for Ranges / default init value
#ifdef FUTURES_HAS_CONCEPTS
        template <class E, class P, class R, class Fun = std::plus<>>
        requires(is_executor_v<E> || is_execution_policy_v<E>)
                && is_range_partitioner_v<P, R> && is_input_range_v<R>
                && is_indirectly_binary_invocable_v<
                    Fun,
                    iterator_t<R>,
                    iterator_t<R>>
                && detail::is_copy_constructible_v<Fun>
#else
        template <
            class E,
            class P,
            class R,
            class Fun = std::plus<>,
            std::enable_if_t<
                (is_executor_v<E>
                 || is_execution_policy_v<E>) &&is_range_partitioner_v<P, R>
                    && is_input_range_v<R>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, P p, R &&r, Fun f = std::plus<>()) const {
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

        /// Overload for Iterators / default parallel executor
#ifdef FUTURES_HAS_CONCEPTS
        template <class P, class I, class S, class T, class Fun = std::plus<>>
        requires is_partitioner_v<P, I, S> && is_input_iterator_v<I>
                 && is_sentinel_for_v<S, I>
                 && detail::is_same_v<iter_value_t<I>, T>
                 && is_indirectly_binary_invocable_v<Fun, I, I>
                 && detail::is_copy_constructible_v<Fun>
#else
        template <
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_partitioner_v<P, I, S> && is_input_iterator_v<I>
                    && is_sentinel_for_v<S, I>
                    && detail::is_same_v<iter_value_t<I>, T>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(P p, I first, S last, T i, Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return Derived{}.run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    i,
                    std::move(f));
            } else {
                return Derived{}.run(
                    make_default_executor(),
                    p,
                    first,
                    last,
                    i,
                    std::move(f));
            }
        }

        /// Overload for Iterators / default parallel executor / default init
        /// value
#ifdef FUTURES_HAS_CONCEPTS
        template <class P, class I, class S, class Fun = std::plus<>>
        requires is_partitioner_v<P, I, S> && is_input_iterator_v<I>
                 && is_sentinel_for_v<S, I>
                 && is_indirectly_binary_invocable_v<Fun, I, I>
                 && detail::is_copy_constructible_v<Fun>
#else
        template <
            class P,
            class I,
            class S,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_partitioner_v<P, I, S> && is_input_iterator_v<I>
                    && is_sentinel_for_v<S, I>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(P p, I first, S last, Fun f = std::plus<>()) const {
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
                    p,
                    first,
                    last,
                    std::move(f));
            }
        }

        /// Overload for Ranges / default parallel executor
#ifdef FUTURES_HAS_CONCEPTS
        template <class P, class R, class T, class Fun = std::plus<>>
        requires is_range_partitioner_v<P, R> && is_input_range_v<R>
                 && detail::is_same_v<range_value_t<R>, T>
                 && is_indirectly_binary_invocable_v<
                     Fun,
                     iterator_t<R>,
                     iterator_t<R>>
                 && detail::is_copy_constructible_v<Fun>
#else
        template <
            class P,
            class R,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_range_partitioner_v<P, R> && is_input_range_v<R>
                    && detail::is_same_v<range_value_t<R>, T>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(P p, R &&r, T i, Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    i,
                    std::move(f));
            } else {
                return operator()(
                    make_default_executor(),
                    p,
                    std::begin(r),
                    std::end(r),
                    i,
                    std::move(f));
            }
        }

        /// Overload for Ranges / default parallel executor / default init value
#ifdef FUTURES_HAS_CONCEPTS
        template <class P, class R, class Fun = std::plus<>>
        requires is_range_partitioner_v<P, R> && is_input_range_v<R>
                 && is_indirectly_binary_invocable_v<
                     Fun,
                     iterator_t<R>,
                     iterator_t<R>>
                 && detail::is_copy_constructible_v<Fun>
#else
        template <
            class P,
            class R,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_range_partitioner_v<P, R> && is_input_range_v<R>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(P p, R &&r, Fun f = std::plus<>()) const {
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

        /// Overload for Iterators / default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <class E, class I, class S, class T, class Fun = std::plus<>>
        requires(is_executor_v<E> || is_execution_policy_v<E>)
                && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                && detail::is_same_v<iter_value_t<I>, T>
                && is_indirectly_binary_invocable_v<Fun, I, I>
                && detail::is_copy_constructible_v<Fun>
#else
        template <
            class E,
            class I,
            class S,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                (is_executor_v<E>
                 || is_execution_policy_v<E>) &&is_input_iterator_v<I>
                    && is_sentinel_for_v<S, I>
                    && detail::is_same_v<iter_value_t<I>, T>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, I first, S last, T i, Fun f = std::plus<>())
            const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    i,
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(first, last),
                    first,
                    last,
                    i,
                    std::move(f));
            }
        }

        /// Overload for Iterators / default partitioner / default init value
#ifdef FUTURES_HAS_CONCEPTS
        template <class E, class I, class S, class Fun = std::plus<>>
        requires(is_executor_v<E> || is_execution_policy_v<E>)
                && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                && is_indirectly_binary_invocable_v<Fun, I, I>
                && detail::is_copy_constructible_v<Fun>
#else
        template <
            class E,
            class I,
            class S,
            class Fun = std::plus<>,
            std::enable_if_t<
                (is_executor_v<E>
                 || is_execution_policy_v<E>) &&is_input_iterator_v<I>
                    && is_sentinel_for_v<S, I>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, I first, S last, Fun f = std::plus<>()) const {
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
#ifdef FUTURES_HAS_CONCEPTS
        template <class E, class R, class T, class Fun = std::plus<>>
        requires(is_executor_v<E> || is_execution_policy_v<E>)
                && is_input_range_v<R> && detail::is_same_v<range_value_t<R>, T>
                && is_indirectly_binary_invocable_v<
                    Fun,
                    iterator_t<R>,
                    iterator_t<R>>
                && detail::is_copy_constructible_v<Fun>
#else
        template <
            class E,
            class R,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                (is_executor_v<E>
                 || is_execution_policy_v<E>) &&is_input_range_v<R>
                    && detail::is_same_v<range_value_t<R>, T>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, R &&r, T i, Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    i,
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(r),
                    std::begin(r),
                    std::end(r),
                    i,
                    std::move(f));
            }
        }

        /// Overload for Ranges / default partitioner / default init value
#ifdef FUTURES_HAS_CONCEPTS
        template <class E, class R, class Fun = std::plus<>>
        requires(is_executor_v<E> || is_execution_policy_v<E>)
                && is_input_range_v<R>
                && is_indirectly_binary_invocable_v<
                    Fun,
                    iterator_t<R>,
                    iterator_t<R>>
                && detail::is_copy_constructible_v<Fun>
#else
        template <
            class E,
            class R,
            class Fun = std::plus<>,
            std::enable_if_t<
                (is_executor_v<E>
                 || is_execution_policy_v<E>) &&is_input_range_v<R>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, R &&r, Fun f = std::plus<>()) const {
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
                    make_default_partitioner(r),
                    std::begin(r),
                    std::end(r),
                    std::move(f));
            }
        }

        /// Overload for Iterators / default executor / default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <class I, class S, class T, class Fun = std::plus<>>
        requires is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                 && detail::is_same_v<iter_value_t<I>, T>
                 && is_indirectly_binary_invocable_v<Fun, I, I>
                 && detail::is_copy_constructible_v<Fun>
#else
        template <
            class I,
            class S,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && detail::is_same_v<iter_value_t<I>, T>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(I first, S last, T i, Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    i,
                    std::move(f));
            } else {
                return operator()(
                    make_default_executor(),
                    make_default_partitioner(first, last),
                    first,
                    last,
                    i,
                    std::move(f));
            }
        }

        /// Iterators / default executor / default partitioner / default
        /// init value
#ifdef FUTURES_HAS_CONCEPTS
        template <class I, class S, class Fun = std::plus<>>
        requires is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                 && is_indirectly_binary_invocable_v<Fun, I, I>
                 && detail::is_copy_constructible_v<Fun>
#else
        template <
            class I,
            class S,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(I first, S last, Fun f = std::plus<>()) const {
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

        /// Overload for Ranges / default executor / default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <class R, class T, class Fun = std::plus<>>
        requires is_input_range_v<R> && detail::is_same_v<range_value_t<R>, T>
                 && is_indirectly_binary_invocable_v<
                     Fun,
                     iterator_t<R>,
                     iterator_t<R>>
                 && detail::is_copy_constructible_v<Fun>
#else
        template <
            class R,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_input_range_v<R> && detail::is_same_v<range_value_t<R>, T>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(R &&r, T i, Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    i,
                    std::move(f));
            } else {
                return operator()(
                    make_default_executor(),
                    make_default_partitioner(r),
                    std::begin(r),
                    std::end(r),
                    i,
                    std::move(f));
            }
        }

        /// Ranges / default executor / default partitioner / default init
        /// value
#ifdef FUTURES_HAS_CONCEPTS
        template <class R, class Fun = std::plus<>>
        requires is_input_range_v<R>
                 && is_indirectly_binary_invocable_v<
                     Fun,
                     iterator_t<R>,
                     iterator_t<R>>
                 && detail::is_copy_constructible_v<Fun>
#else
        template <
            class R,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_input_range_v<R>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(R &&r, Fun f = std::plus<>()) const {
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

#endif // FUTURES_ALGORITHM_TRAITS_BINARY_INVOKE_ALGORITHM_HPP
