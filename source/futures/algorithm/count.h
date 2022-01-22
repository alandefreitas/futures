//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_COUNT_H
#define FUTURES_COUNT_H

#include <execution>
#include <variant>

#include <futures/algorithm/detail/traits/range/range/concepts.h>

#include <futures/futures.h>
#include <futures/algorithm/traits/algorithm_traits.h>
#include <futures/algorithm/detail/try_async.h>
#include <futures/algorithm/partitioner/partitioner.h>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref count function
    class count_functor : public detail::value_cmp_algorithm_functor<count_functor> {
      public:
        /// \brief Complete overload of the count algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam T Value to compare
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param value Value
        /// \brief function template \c count
        template <class E, class P, class I, class S, class T,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && is_input_iterator_v<I> &&
                                       futures::detail::sentinel_for<S, I> &&
                                       futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T*, I>,
                                   int> = 0>
        futures::detail::iter_difference_t<I> run(const E &ex, P p, I first, S last, T v) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::count(first, last, v);
            }

            // Run count on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, v); });

            // Run count on lhs: [first, middle]
            bool lhs = operator()(ex, p, first, middle, v);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs + rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                return lhs + operator()(make_inline_executor(), p, middle, last, v);
            }
        }
    };

    /// \brief Returns the number of elements matching an element
    inline constexpr count_functor count;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_COUNT_H
