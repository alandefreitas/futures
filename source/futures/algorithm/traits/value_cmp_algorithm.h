//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_VALUE_CMP_ALGORITHM_H
#define FUTURES_ALGORITHM_VALUE_CMP_ALGORITHM_H

/// \file Identify traits for algorithms, like we do for other types
///
/// The traits help us generate auxiliary algorithm overloads
/// This is somewhat similar to the pattern of traits and algorithms for ranges
/// and views It allows us to get algorithm overloads for free, including
/// default inference of the best execution policies
///
/// \see https://en.cppreference.com/w/cpp/ranges/transform_view
/// \see https://en.cppreference.com/w/cpp/ranges/view
///

#include <futures/algorithm/partitioner/partitioner.h>
#include <futures/algorithm/policies.h>
#include <futures/executor/default_executor.h>
#include <futures/executor/inline_executor.h>
#include <futures/algorithm/detail/traits/range/range/concepts.h>
#include <execution>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup algorithm-traits Algorithm Traits
     *  @{
     */

    /// \brief CRTP class with the overloads for classes that look for
    /// elements in a sequence with an unary function This includes
    /// algorithms such as for_each, any_of, all_of, ...
    template <class Derived>
    class value_cmp_algorithm_functor
    {
    public:
        template <
            class E,
            class P,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                is_executor_v<
                    E> && is_partitioner_v<P, I, S> && is_input_iterator_v<I> && futures::detail::sentinel_for<S, I> && futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I>,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, I first, S last, T f) const {
            return Derived().run(ex, std::forward<P>(p), first, last, f);
        }

        /// \overload execution policy instead of executor
        /// we can't however, count on std::is_execution_policy being defined
        template <
            class E,
            class P,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                !is_executor_v<
                    E> && is_execution_policy_v<E> && is_partitioner_v<P, I, S> && is_input_iterator_v<I> && futures::detail::sentinel_for<S, I> && futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I>,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &, P p, I first, S last, T f) const {
            return Derived().operator()(
                make_policy_executor<E, I, S>(),
                std::forward<P>(p),
                first,
                last,
                f);
        }

        /// \overload Ranges
        template <
            class E,
            class P,
            class R,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                (is_executor_v<E> || is_execution_policy_v<E>) &&is_range_partitioner_v<
                    P,
                    R> && futures::detail::input_range<R> && futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, futures::detail::iterator_t<R>> && std::is_copy_constructible_v<T>,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, R &&r, T f) const {
            return Derived().operator()(
                ex,
                std::forward<P>(p),
                std::begin(r),
                std::end(r),
                std::move(f));
        }

        /// \overload Iterators / default parallel executor
        template <
            class P,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                is_partitioner_v<
                    P,
                    I,
                    S> && is_input_iterator_v<I> && futures::detail::sentinel_for<S, I> && futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I> && std::is_copy_constructible_v<T>,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, I first, S last, T f) const {
            return Derived().operator()(
                make_default_executor(),
                std::forward<P>(p),
                first,
                last,
                std::move(f));
        }

        /// \overload Ranges / default parallel executor
        template <
            class P,
            class R,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                is_range_partitioner_v<
                    P,
                    R> && futures::detail::input_range<R> && futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, futures::detail::iterator_t<R>> && std::is_copy_constructible_v<T>,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, R &&r, T f) const {
            return Derived().operator()(
                make_default_executor(),
                std::forward<P>(p),
                std::begin(r),
                std::end(r),
                std::move(f));
        }

        /// \overload Iterators / default partitioner
        template <
            class E,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                (is_executor_v<E> || is_execution_policy_v<E>) &&is_input_iterator_v<
                    I> && futures::detail::sentinel_for<S, I> && futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I>,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, I first, S last, T f) const {
            return Derived().operator()(
                ex,
                make_default_partitioner(first, last),
                first,
                last,
                std::move(f));
        }

        /// \overload Ranges / default partitioner
        template <
            class E,
            class R,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                (is_executor_v<E> || is_execution_policy_v<E>) &&futures::detail::input_range<
                    R> && futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, futures::detail::iterator_t<R>> && std::is_copy_constructible_v<T>,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, R &&r, T f) const {
            return Derived().operator()(
                ex,
                make_default_partitioner(std::forward<R>(r)),
                std::begin(r),
                std::end(r),
                std::move(f));
        }

        /// \overload Iterators / default executor / default partitioner
        template <
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                is_input_iterator_v<
                    I> && futures::detail::sentinel_for<S, I> && futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I>,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(I first, S last, T f) const {
            return Derived().operator()(
                make_default_executor(),
                make_default_partitioner(first, last),
                first,
                last,
                std::move(f));
        }

        /// \overload Ranges / default executor / default partitioner
        template <
            class R,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                futures::detail::input_range<
                    R> && futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, futures::detail::iterator_t<R>> && std::is_copy_constructible_v<T>,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(R &&r, T f) const {
            return Derived().operator()(
                make_default_executor(),
                make_default_partitioner(r),
                std::begin(r),
                std::end(r),
                std::move(f));
        }
    };
    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_VALUE_CMP_ALGORITHM_H
