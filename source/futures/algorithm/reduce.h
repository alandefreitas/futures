//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_REDUCE_H
#define FUTURES_REDUCE_H

#include <execution>
#include <variant>
#include <numeric>

#include <futures/algorithm/detail/traits/range/range/concepts.h>

#include <futures/futures.h>
#include <futures/algorithm/traits/algorithm_traits.h>
#include <futures/algorithm/detail/try_async.h>
#include <futures/algorithm/partitioner/partitioner.h>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref reduce function
    class reduce_functor {
      public:
        /// \brief Complete overload of the reduce algorithm
        /// The reduce algorithm is equivalent to a version std::accumulate where the binary operation
        /// is applied out of order.
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param i Initial value for the reduction
        /// \param f Function
        template <
            class E, class P, class I, class S, class T, class Fun = std::plus<>,
            std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                 futures::detail::sentinel_for<S, I> && std::is_same_v<futures::detail::iter_value_t<I>, T> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        T operator()(const E &ex, P p, I first, S last, T i, Fun f = std::plus<>()) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::reduce(first, last, i, f);
            }

            // Run reduce on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() {
                return operator()(ex, p, middle, last, i, f);
            });

            // Run reduce on lhs: [first, middle]
            T lhs = operator()(ex, p, first, middle, i, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return f(lhs, rhs.get());
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                T i_rhs = operator()(make_inline_executor(), p, middle, last, i, f);
                return f(lhs, i_rhs);
            }
        }

        /// \overload default init value
        template <class E, class P, class I, class S, class Fun = std::plus<>,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> && futures::detail::indirectly_binary_invocable_<Fun, I, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0>
        futures::detail::iter_value_t<I> operator()(const E &ex, P p, I first, S last, Fun f = std::plus<>()) const {
            if (first != last) {
                return operator()(ex, std::forward<P>(p), std::next(first), last, *first, f);
            } else {
                return futures::detail::iter_value_t<I>{};
            }
        }

        /// \overload execution policy instead of executor
        template <
            class E, class P, class I, class S, class T, class Fun = std::plus<>,
            std::enable_if_t<not is_executor_v<E> && is_execution_policy_v<E> && is_partitioner_v<P, I, S> &&
                                 futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 std::is_same_v<futures::detail::iter_value_t<I>, T> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        T operator()(const E &, P p, I first, S last, T i, Fun f = std::plus<>()) const {
            return operator()(make_policy_executor<E, I, S>(), std::forward<P>(p), first, last, i, f);
        }

        /// \overload execution policy instead of executor / default init value
        template <
            class E, class P, class I, class S, class Fun = std::plus<>,
            std::enable_if_t<not is_executor_v<E> && is_execution_policy_v<E> && is_partitioner_v<P, I, S> &&
                                 futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        futures::detail::iter_value_t<I> operator()(const E &, P p, I first, S last, Fun f = std::plus<>()) const {
            return operator()(make_policy_executor<E, I, S>(), std::forward<P>(p), first, last, f);
        }

        /// \overload Ranges
        template <class E, class P, class R, class T, class Fun = std::plus<>,
                  std::enable_if_t<
                      (is_executor_v<E> || is_execution_policy_v<E>)&&is_range_partitioner_v<P, R> &&
                          futures::detail::input_range<R> && std::is_same_v<futures::detail::range_value_t<R>, T> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        T operator()(const E &ex, P p, R &&r, T i, Fun f = std::plus<>()) const {
            return operator()(ex, std::forward<P>(p), std::begin(r), std::end(r), i, std::move(f));
        }

        /// \overload Ranges / default init value
        template <class E, class P, class R, class Fun = std::plus<>,
                  std::enable_if_t<
                      (is_executor_v<E> || is_execution_policy_v<E>)&&is_range_partitioner_v<P, R> &&
                          futures::detail::input_range<R> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        futures::detail::range_value_t<R> operator()(const E &ex, P p, R &&r, Fun f = std::plus<>()) const {
            return operator()(ex, std::forward<P>(p), std::begin(r), std::end(r), std::move(f));
        }

        /// \overload Iterators / default parallel executor
        template <
            class P, class I, class S, class T, class Fun = std::plus<>,
            std::enable_if_t<is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 std::is_same_v<futures::detail::iter_value_t<I>, T> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        T operator()(P p, I first, S last, T i, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), std::forward<P>(p), first, last, i, std::move(f));
        }

        /// \overload Iterators / default parallel executor / default init value
        template <
            class P, class I, class S, class Fun = std::plus<>,
            std::enable_if_t<is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        futures::detail::iter_value_t<I> operator()(P p, I first, S last, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), std::forward<P>(p), first, last, std::move(f));
        }

        /// \overload Ranges / default parallel executor
        template <
            class P, class R, class T, class Fun = std::plus<>,
            std::enable_if_t<
                is_range_partitioner_v<P, R> && futures::detail::input_range<R> && std::is_same_v<futures::detail::range_value_t<R>, T> &&
                    futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                    std::is_copy_constructible_v<Fun>,
                int> = 0>
        T operator()(P p, R &&r, T i, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), std::forward<P>(p), std::begin(r), std::end(r), i, std::move(f));
        }

        /// \overload Ranges / default parallel executor / default init value
        template <class P, class R, class Fun = std::plus<>,
                  std::enable_if_t<
                      is_range_partitioner_v<P, R> && futures::detail::input_range<R> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        futures::detail::range_value_t<R> operator()(P p, R &&r, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), std::forward<P>(p), std::begin(r), std::end(r), std::move(f));
        }

        /// \overload Iterators / default partitioner
        template <
            class E, class I, class S, class T, class Fun = std::plus<>,
            std::enable_if_t<(is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_iterator<I> &&
                                 futures::detail::sentinel_for<S, I> && std::is_same_v<futures::detail::iter_value_t<I>, T> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        T operator()(const E &ex, I first, S last, T i, Fun f = std::plus<>()) const {
            return operator()(ex, make_default_partitioner(first, last), first, last, i, std::move(f));
        }

        /// \overload Iterators / default partitioner / default init value
        template <class E, class I, class S, class Fun = std::plus<>,
                  std::enable_if_t<(is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> && futures::detail::indirectly_binary_invocable_<Fun, I, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0>
        futures::detail::iter_value_t<I> operator()(const E &ex, I first, S last, Fun f = std::plus<>()) const {
            return operator()(ex, make_default_partitioner(first, last), first, last, std::move(f));
        }

        /// \overload Ranges / default partitioner
        template <class E, class R, class T, class Fun = std::plus<>,
                  std::enable_if_t<
                      (is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_range<R> &&
                          std::is_same_v<futures::detail::range_value_t<R>, T> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        T operator()(const E &ex, R &&r, T i, Fun f = std::plus<>()) const {
            return operator()(ex, make_default_partitioner(std::forward<R>(r)), std::begin(r), std::end(r), i, std::move(f));
        }

        /// \overload Ranges / default partitioner / default init value
        template <class E, class R, class Fun = std::plus<>,
                  std::enable_if_t<
                      (is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_range<R> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        futures::detail::range_value_t<R> operator()(const E &ex, R &&r, Fun f = std::plus<>()) const {
            return operator()(ex, make_default_partitioner(std::forward<R>(r)), std::begin(r), std::end(r), std::move(f));
        }

        /// \overload Iterators / default executor / default partitioner
        template <
            class I, class S, class T, class Fun = std::plus<>,
            std::enable_if_t<futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 std::is_same_v<futures::detail::iter_value_t<I>, T> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        T operator()(I first, S last, T i, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), make_default_partitioner(first, last), first, last, i, std::move(f));
        }

        /// \overload Iterators / default executor / default partitioner / default init value
        template <
            class I, class S, class Fun = std::plus<>,
            std::enable_if_t<futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        futures::detail::iter_value_t<I> operator()(I first, S last, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), make_default_partitioner(first, last), first, last, std::move(f));
        }

        /// \overload Ranges / default executor / default partitioner
        template <class R, class T, class Fun = std::plus<>,
                  std::enable_if_t<
                      futures::detail::input_range<R> && std::is_same_v<futures::detail::range_value_t<R>, T> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        T operator()(R &&r, T i, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), make_default_partitioner(r), std::begin(r), std::end(r), i,
                       std::move(f));
        }

        /// \overload Ranges / default executor / default partitioner / default init value
        template <class R, class Fun = std::plus<>,
                  std::enable_if_t<
                      futures::detail::input_range<R> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        futures::detail::range_value_t<R> operator()(R &&r, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), make_default_partitioner(r), std::begin(r), std::end(r), std::move(f));
        }
    };

    /// \brief Sums up (or accumulate with a custom function) a range of elements, except out of order
    inline constexpr reduce_functor reduce;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_REDUCE_H
