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

    /// Binary algorithm overloads
    /**
     * CRTP class with the overloads for algorithms that aggregate
     * elements in a sequence with an binary function.
     *
     * This includes algorithms such as @ref reduce and @ref accumulate.
     */
    template <class Derived>
    class binary_invoke_algorithm_functor {
    public:
        /// Execute the underlying algorithm
        /**
         *  \tparam E Executor type
         *  \tparam I Iterator type
         *  \tparam S Sentinel type
         *  \tparam P Partitioner type
         *  \tparam T Type of value to aggregate the elements with
         *  \tparam Fun Function type
         *  \param ex An executor instance
         *  \param p A partitioner instance
         *  \param first Iterator to first element in the range
         *  \param last Sentinel iterator to one element past the last
         *  \param value initial value to aggregate the elements with
         *  \param f Function invocable with the return type of the iterator
         *  \return Result of the underlying algorithm
         */
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            std::convertible_to<std::iter_value_t<I>> T,
            std::indirect_binary_predicate<I, T const *> Fun = std::plus<>>
#else
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_executor_v<E> && is_partitioner_for_v<P, I, S>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_convertible_to_v<T, iter_value_t<I>>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(
            E const &ex,
            P p,
            I first,
            S last,
            T const &value,
            Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return Derived().run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    value,
                    std::move(f));
            } else {
                return Derived().run(ex, p, first, last, value, std::move(f));
            }
        }

        /// Execute the algorithm with the default initialization value
        /**
         * The default initialization value is always the first element in the
         * sequence, while the algorithm is executed with other elements.
         */
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            std::indirect_binary_predicate<I, I> Fun = std::plus<>>
#else
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_executor_v<E> && is_partitioner_for_v<P, I, S>
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

        /// Execute the underlying algorithm with an execution policy
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            std::convertible_to<std::iter_value_t<I>> T,
            std::indirect_binary_predicate<I, T const *> Fun = std::plus<>>
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
                    && is_partitioner_for_v<P, I, S> && is_input_iterator_v<I>
                    && is_sentinel_for_v<S, I>
                    && is_convertible_to_v<T, iter_value_t<I>>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(
            E const &,
            P p,
            I first,
            S last,
            T const &value,
            Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    value,
                    std::move(f));
            } else {
                return operator()(
                    make_policy_executor<E, I, S>(),
                    p,
                    first,
                    last,
                    value,
                    std::move(f));
            }
        }

        /// Execute the algorithm with an execution policy and default value
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            std::indirect_binary_predicate<I, I> Fun = std::plus<>>
#else
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun = std::plus<>,
            std::enable_if_t<
                (!is_executor_v<E> && is_execution_policy_v<E>
                 && is_partitioner_for_v<P, I, S> && is_input_iterator_v<I>
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

        /// Execute the algorithm on a range of iterators
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::ranges::range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            std::convertible_to<std::ranges::range_value_t<R>> T,
            std::indirect_binary_predicate<std::ranges::iterator_t<R>, T const *>
                Fun
            = std::plus<>>
#else
        template <
            class E,
            class P,
            class R,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
                    && is_partitioner_for_v<P, iterator_t<R>> && is_input_range_v<R>
                    && is_convertible_to_v<T, range_value_t<R>>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(
            E const &ex,
            P p,
            R &&r,
            T const &value,
            Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    p,
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            }
        }

        /// Execute the algorithm on a range with an execution policy
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::ranges::range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            std::convertible_to<std::ranges::range_value_t<R>> T,
            std::indirect_binary_predicate<std::ranges::iterator_t<R>, T const *>
                Fun
            = std::plus<>>
#else
        template <
            class E,
            class P,
            class R,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_partitioner_for_v<P, iterator_t<R>> && is_input_range_v<R>
                    && is_convertible_to_v<T, range_value_t<R>>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(
            E const &ex,
            P p,
            R &&r,
            T const &value,
            Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    p,
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            }
        }

        /// Execute algorithm on a range with default initialization value
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::ranges::range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            std::indirect_binary_predicate<
                std::ranges::iterator_t<R>,
                std::ranges::iterator_t<R>> Fun
            = std::plus<>>
#else
        template <
            class E,
            class P,
            class R,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
                    && is_partitioner_for_v<P, iterator_t<R>> && is_input_range_v<R>
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

        /// Execute algorithm on a range with policy and default value
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::ranges::range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            std::indirect_binary_predicate<
                std::ranges::iterator_t<R>,
                std::ranges::iterator_t<R>> Fun
            = std::plus<>>
#else
        template <
            class E,
            class P,
            class R,
            class Fun = std::plus<>,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_partitioner_for_v<P, iterator_t<R>> && is_input_range_v<R>
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

        /// Execute underlying algorithm with default executor
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            std::convertible_to<std::iter_value_t<I>> T,
            std::indirect_binary_predicate<I, T const *> Fun = std::plus<>>
#else
        template <
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_partitioner_for_v<P, I, S> && is_input_iterator_v<I>
                    && is_sentinel_for_v<S, I>
                    && is_convertible_to_v<T, iter_value_t<I>>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(P p, I first, S last, T value, Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return Derived{}.run(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    value,
                    std::move(f));
            } else {
                return Derived{}.run(
                    make_default_executor(),
                    p,
                    first,
                    last,
                    value,
                    std::move(f));
            }
        }

        /// Execute algorithm with default executor and default value
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::input_iterator I,
            std::sentinel_for<I> S,
            partitioner_for<I, S> P,
            std::indirect_binary_predicate<I, I> Fun = std::plus<>>
#else
        template <
            class P,
            class I,
            class S,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_partitioner_for_v<P, I, S> && is_input_iterator_v<I>
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

        /// Execute algorithm on range with default executor
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::ranges::range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            std::convertible_to<std::ranges::range_value_t<R>> T,
            std::indirect_binary_predicate<std::ranges::iterator_t<R>, T const *>
                Fun
            = std::plus<>>
#else
        template <
            class P,
            class R,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_partitioner_for_v<P, iterator_t<R>> && is_input_range_v<R>
                    && is_convertible_to_v<T, range_value_t<R>>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        T const *>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(P p, R &&r, T const &value, Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            } else {
                return operator()(
                    make_default_executor(),
                    p,
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            }
        }

        /// Execute algorithm on range with default executor and initialization
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::ranges::range R,
            partitioner_for<std::ranges::iterator_t<R>> P,
            std::indirect_binary_predicate<
                std::ranges::iterator_t<R>,
                std::ranges::iterator_t<R>> Fun
            = std::plus<>>
#else
        template <
            class P,
            class R,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_partitioner_for_v<P, iterator_t<R>> && is_input_range_v<R>
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

        /// Execute algorithm with default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            std::convertible_to<std::iter_value_t<I>> T,
            std::indirect_binary_predicate<I, T const *> Fun = std::plus<>>
#else
        template <
            class E,
            class I,
            class S,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_convertible_to_v<T, iter_value_t<I>>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(
            E const &ex,
            I first,
            S last,
            T const &value,
            Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    value,
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(first, last),
                    first,
                    last,
                    value,
                    std::move(f));
            }
        }

        /// Execute algorithm with execution policy and default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            std::convertible_to<std::iter_value_t<I>> T,
            std::indirect_binary_predicate<I, T const *> Fun = std::plus<>>
#else
        template <
            class E,
            class I,
            class S,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_convertible_to_v<T, iter_value_t<I>>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(
            E const &ex,
            I first,
            S last,
            T const &value,
            Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    value,
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(first, last),
                    first,
                    last,
                    value,
                    std::move(f));
            }
        }

        /// Execute algorithm with default partitioner and initialization value
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            std::indirect_binary_predicate<I, I> Fun = std::plus<>>
#else
        template <
            class E,
            class I,
            class S,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
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

        /// Execute algorithm with policy, default partitioner and default value
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::input_iterator I,
            std::sentinel_for<I> S,
            std::indirect_binary_predicate<I, I> Fun = std::plus<>>
#else
        template <
            class E,
            class I,
            class S,
            class Fun = std::plus<>,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
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

        /// Execute algorithm on range with default partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::ranges::input_range R,
            std::convertible_to<std::ranges::range_value_t<R>> T,
            std::indirect_binary_predicate<std::ranges::iterator_t<R>, T const *>
                Fun
            = std::plus<>>
#else
        template <
            class E,
            class R,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
                    && is_input_range_v<R>
                    && is_convertible_to_v<T, range_value_t<R>>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        T const *>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, R &&r, T const &value, Fun f = std::plus<>())
            const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(r),
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            }
        }

        /// Execute algorithm on range with execution policy and default
        /// partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::ranges::input_range R,
            std::convertible_to<std::ranges::range_value_t<R>> T,
            std::indirect_binary_predicate<std::ranges::iterator_t<R>, T const *>
                Fun
            = std::plus<>>
#else
        template <
            class E,
            class R,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
                    && is_input_range_v<R>
                    && is_convertible_to_v<T, range_value_t<R>>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        T const *>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(E const &ex, R &&r, T const &value, Fun f = std::plus<>())
            const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            } else {
                return operator()(
                    ex,
                    make_default_partitioner(r),
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            }
        }

        /// Execute algorithm on range with default partitioner and value
#ifdef FUTURES_HAS_CONCEPTS
        template <
            executor E,
            std::ranges::input_range R,
            std::indirect_binary_predicate<
                std::ranges::iterator_t<R>,
                std::ranges::iterator_t<R>> Fun
            = std::plus<>>
#else
        template <
            class E,
            class R,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_executor_v<E> && !is_execution_policy_v<E>
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

        /// Execute algorithm on range with policy, default partitioner and value
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::ranges::input_range R,
            std::indirect_binary_predicate<
                std::ranges::iterator_t<R>,
                std::ranges::iterator_t<R>> Fun
            = std::plus<>>
#else
        template <
            class E,
            class R,
            class Fun = std::plus<>,
            std::enable_if_t<
                !is_executor_v<E> && is_execution_policy_v<E>
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

        /// Execute algorithm with default executor and partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::input_iterator I,
            std::sentinel_for<I> S,
            std::convertible_to<std::iter_value_t<I>> T,
            std::indirect_binary_predicate<I, I> Fun = std::plus<>>
#else
        template <
            class I,
            class S,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_input_iterator_v<I> && is_sentinel_for_v<S, I>
                    && is_convertible_to_v<T, iter_value_t<I>>
                    && is_indirectly_binary_invocable_v<Fun, I, I>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(I first, S last, T const &value, Fun f = std::plus<>())
            const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    first,
                    last,
                    value,
                    std::move(f));
            } else {
                return operator()(
                    make_default_executor(),
                    make_default_partitioner(first, last),
                    first,
                    last,
                    value,
                    std::move(f));
            }
        }

        /// Execute algorithm with default executor, partitioner, and value
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::input_iterator I,
            std::sentinel_for<I> S,
            std::indirect_binary_predicate<I, I> Fun = std::plus<>>
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

        /// Execute algorithm on range with default executor and partitioner
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::ranges::input_range R,
            std::convertible_to<std::ranges::range_value_t<R>> T,
            std::indirect_binary_predicate<std::ranges::iterator_t<R>, T const *>
                Fun
            = std::plus<>>
#else
        template <
            class R,
            class T,
            class Fun = std::plus<>,
            std::enable_if_t<
                is_input_range_v<R> && is_convertible_to_v<T, range_value_t<R>>
                    && is_indirectly_binary_invocable_v<
                        Fun,
                        iterator_t<R>,
                        iterator_t<R>>
                    && detail::is_copy_constructible_v<Fun>,
                int>
            = 0>
#endif
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR FUTURES_DETAIL(decltype(auto))
        operator()(R &&r, T const &value, Fun f = std::plus<>()) const {
            if (detail::is_constant_evaluated()) {
                return operator()(
                    make_inline_executor(),
                    halve_partitioner(1),
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            } else {
                return operator()(
                    make_default_executor(),
                    make_default_partitioner(r),
                    std::begin(r),
                    std::end(r),
                    value,
                    std::move(f));
            }
        }

        /// Execute algorithm on range with default executor, partitioner, and
        /// value
#ifdef FUTURES_HAS_CONCEPTS
        template <
            std::ranges::input_range R,
            std::indirect_binary_predicate<
                std::ranges::iterator_t<R>,
                std::ranges::iterator_t<R>> Fun
            = std::plus<>>
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
