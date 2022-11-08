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
 *  @see https://en.cppreference.com/w/cpp/ranges/transform_view
 *  @see https://en.cppreference.com/w/cpp/ranges/view
 */

#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/policies.hpp>
#include <futures/algorithm/traits/is_indirectly_unary_invocable.hpp>
#include <futures/algorithm/traits/is_input_range.hpp>
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

    /// Overloads for unary invoke algorithms
    /**
     * CRTP class with the overloads for classes that look for
     * elements in a sequence with an unary function.
     *
     * This includes algorithms such as for_each, any_of, all_of, ...
     */
    template <class Derived>
    class unary_invoke_algorithm_functor {
    public:
        FUTURES_TEMPLATE(class E, class P, class I, class S, class Fun)
        (requires((
            is_executor_v<E> && is_partitioner_v<P, I, S>
            && is_input_iterator_v<I> && is_sentinel_for_v<S, I>
            && is_indirectly_unary_invocable_v<Fun, I>
            && std::is_copy_constructible_v<Fun>) ))
            FUTURES_CONSTANT_EVALUATED_CONSTEXPR
            FUTURES_DETAIL(FUTURES_DETAIL(decltype(auto)))
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

        /// @overload execution policy instead of executor
        /// we can't however, count on std::is_execution_policy being defined
        FUTURES_TEMPLATE(class E, class P, class I, class S, class Fun)
        (requires((
            !is_executor_v<E> && is_execution_policy_v<E>
            && is_partitioner_v<P, I, S> && is_input_iterator_v<I>
            && is_sentinel_for_v<S, I>
            && is_indirectly_unary_invocable_v<Fun, I>
            && std::is_copy_constructible_v<Fun>) ))
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

        /// Overload for Ranges
        FUTURES_TEMPLATE(class E, class P, class R, class Fun)
        (requires((
            (is_executor_v<E>
             || is_execution_policy_v<E>) &&is_range_partitioner_v<P, R>
            && is_input_range_v<R>
            && is_indirectly_unary_invocable_v<Fun, iterator_t<R>>
            && std::is_copy_constructible_v<Fun>) ))
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

        /// Overload for Iterators / default parallel executor
        FUTURES_TEMPLATE(class P, class I, class S, class Fun)
        (requires((
            is_partitioner_v<P, I, S> && is_input_iterator_v<I>
            && is_sentinel_for_v<S, I>
            && is_indirectly_unary_invocable_v<Fun, I>
            && std::is_copy_constructible_v<Fun>) ))
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

        /// Overload for Ranges / default parallel executor
        FUTURES_TEMPLATE(class P, class R, class Fun)
        (requires(
            (is_range_partitioner_v<P, R> && is_input_range_v<R>
             && is_indirectly_unary_invocable_v<Fun, iterator_t<R>>
             && std::is_copy_constructible_v<Fun>) ))
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

        /// Overload for Iterators / default partitioner
        FUTURES_TEMPLATE(class E, class I, class S, class Fun)
        (requires((
            (is_executor_v<E>
             || is_execution_policy_v<E>) &&is_input_iterator_v<I>
            && is_sentinel_for_v<S, I>
            && is_indirectly_unary_invocable_v<Fun, I>
            && std::is_copy_constructible_v<Fun>) ))
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

        /// Overload for Ranges / default partitioner
        FUTURES_TEMPLATE(class E, class R, class Fun)
        (requires((
            (is_executor_v<E> || is_execution_policy_v<E>) &&is_input_range_v<R>
            && is_indirectly_unary_invocable_v<Fun, iterator_t<R>>
            && std::is_copy_constructible_v<Fun>) ))
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

        /// Overload for Iterators / default executor / default partitioner
        FUTURES_TEMPLATE(class I, class S, class Fun)
        (requires(
            (is_input_iterator_v<I> && is_sentinel_for_v<S, I>
             && is_indirectly_unary_invocable_v<Fun, I>
             && std::is_copy_constructible_v<Fun>) ))
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

        /// Overload for Ranges / default executor / default partitioner
        FUTURES_TEMPLATE(class R, class Fun)
        (requires(
            (is_input_range_v<R>
             && is_indirectly_unary_invocable_v<Fun, iterator_t<R>>
             && std::is_copy_constructible_v<Fun>) ))
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
