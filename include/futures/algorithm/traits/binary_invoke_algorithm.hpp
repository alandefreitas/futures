//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_BINARY_INVOKE_ALGORITHM_HPP
#define FUTURES_ALGORITHM_TRAITS_BINARY_INVOKE_ALGORITHM_HPP

#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/policies.hpp>
#include <futures/algorithm/traits/is_indirectly_binary_invocable.hpp>
#include <futures/algorithm/traits/is_input_range.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/futures.hpp>
#include <execution>
#include <numeric>
#include <variant>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup algorithm-traits Algorithm Traits
     *  @{
     */

    /// Binary algorithm overloads
    ///
    /// CRTP class with the overloads for classes that aggregate
    /// elements in a sequence with an binary function. This includes
    /// algorithms such as reduce and accumulate.
    template <class Derived>
    class binary_invoke_algorithm_functor
    {
    public:
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                std::is_same_v<iter_value_t<I>, T> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, I first, S last, T i, Fun f = std::plus<>())
            const {
            return Derived().run(ex, p, first, last, i, f);
        }

        /// @overload default init value
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, I first, S last, Fun f = std::plus<>())
            const {
            if (first != last) {
                return Derived().run(ex, p, std::next(first), last, *first, f);
            } else {
                return iter_value_t<I>{};
            }
        }

        /// @overload execution policy instead of executor
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                !is_executor_v<E> &&
                is_execution_policy_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                std::is_same_v<iter_value_t<I>, T> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &, P p, I first, S last, T i, Fun f = std::plus<>())
            const {
            return Derived().
            operator()(make_policy_executor<E, I, S>(), p, first, last, i, f);
        }

        /// @overload execution policy instead of executor / default init value
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                !is_executor_v<E> &&
                is_execution_policy_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &, P p, I first, S last, Fun f = std::plus<>())
            const {
            return
            operator()(make_policy_executor<E, I, S>(), p, first, last, f);
        }

        /// @overload Ranges
        template <
            class E,
            class P,
            class R,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_range_partitioner_v<P,R> &&
                is_input_range_v<R> &&
                std::is_same_v<range_value_t<R>, T> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, R &&r, T i, Fun f = std::plus<>()) const {
            return Derived().
            operator()(ex, p, std::begin(r), std::end(r), i, std::move(f));
        }

        /// @overload Ranges / default init value
        template <
            class E,
            class P,
            class R,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_range_partitioner_v<P,R> &&
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, R &&r, Fun f = std::plus<>()) const {
            return operator()(ex, p, std::begin(r), std::end(r), std::move(f));
        }

        /// @overload Iterators / default parallel executor
        template <
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format on
                is_partitioner_v<
                    P,
                    I,
                    S> && is_input_iterator_v<I> && is_sentinel_for_v<S, I> && std::is_same_v<iter_value_t<I>, T> && is_indirectly_binary_invocable_v<Fun, I, I> && std::is_copy_constructible_v<Fun>
                // clang-format off
                , int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, I first, S last, T i, Fun f = std::plus<>()) const {
            return
            Derived().run(make_default_executor(), p, first, last, i, std::move(f));
        }

        /// @overload Iterators / default parallel executor / default init value
        template <
            class P,
            class I,
            class S,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_partitioner_v<P,I,S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, I first, S last, Fun f = std::plus<>()) const {
            return
            operator()(make_default_executor(), p, first, last, std::move(f));
        }

        /// @overload Ranges / default parallel executor
        template <
            class P,
            class R,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_range_partitioner_v<P,R> &&
                is_input_range_v<R> &&
                std::is_same_v<range_value_t<R>, T> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, R &&r, T i, Fun f = std::plus<>()) const {
            return Derived().run(
                make_default_executor(),
                p,
                std::begin(r),
                std::end(r),
                i,
                std::move(f));
        }

        /// @overload Ranges / default parallel executor / default init value
        template <
            class P,
            class R,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_range_partitioner_v<P, R> &&
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, R &&r, Fun f = std::plus<>()) const {
            return operator()(
                make_default_executor(),
                p,
                std::begin(r),
                std::end(r),
                std::move(f));
        }

        /// @overload Iterators / default partitioner
        template <
            class E,
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                std::is_same_v<iter_value_t<I>, T> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, I first, S last, T i, Fun f = std::plus<>())
            const {
            return operator()(
                ex,
                make_default_partitioner(first, last),
                first,
                last,
                i,
                std::move(f));
        }

        /// @overload Iterators / default partitioner / default init value
        template <
            class E,
            class I,
            class S,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, I first, S last, Fun f = std::plus<>()) const {
            return operator()(
                ex,
                make_default_partitioner(first, last),
                first,
                last,
                std::move(f));
        }

        /// @overload Ranges / default partitioner
        template <
            class E,
            class R,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_range_v<R> &&
                std::is_same_v<range_value_t<R>, T> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, R &&r, T i, Fun f = std::plus<>()) const {
            return operator()(
                ex,
                make_default_partitioner(std::forward<R>(r)),
                std::begin(r),
                std::end(r),
                i,
                std::move(f));
        }

        /// @overload Ranges / default partitioner / default init value
        template <
            class E,
            class R,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, R &&r, Fun f = std::plus<>()) const {
            return operator()(
                ex,
                make_default_partitioner(std::forward<R>(r)),
                std::begin(r),
                std::end(r),
                std::move(f));
        }

        /// @overload Iterators / default executor / default partitioner
        template <
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                std::is_same_v<iter_value_t<I>, T> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(I first, S last, T i, Fun f = std::plus<>()) const {
            return Derived().run(
                make_default_executor(),
                make_default_partitioner(first, last),
                first,
                last,
                i,
                std::move(f));
        }

        /// @overload Iterators / default executor / default partitioner /
        /// default init value
        template <
            class I,
            class S,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(I first, S last, Fun f = std::plus<>()) const {
            return operator()(
                make_default_executor(),
                make_default_partitioner(first, last),
                first,
                last,
                std::move(f));
        }

        /// @overload Ranges / default executor / default partitioner
        template <
            class R,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_range_v<R> &&
                std::is_same_v<range_value_t<R>, T> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(R &&r, T i, Fun f = std::plus<>()) const {
            return Derived().run(
                make_default_executor(),
                make_default_partitioner(r),
                std::begin(r),
                std::end(r),
                i,
                std::move(f));
        }

        /// @overload Ranges / default executor / default partitioner / default
        /// init value
        template <
            class R,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(R &&r, Fun f = std::plus<>()) const {
            return operator()(
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

#endif // FUTURES_ALGORITHM_TRAITS_BINARY_INVOKE_ALGORITHM_HPP
