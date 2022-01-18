//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_FIND_H
#define FUTURES_FIND_H

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

    /// \brief Functor representing the overloads for the @ref find function
    class find_functor : public detail::value_cmp_algorithm_functor<find_functor> {
      public:
        /// \brief Complete overload of the find algorithm
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
        /// \brief function template \c find
        template <class E, class P, class I, class S, class T,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> &&
                                       futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I>,
                                   int> = 0>
        I run(const E &ex, P p, I first, S last, const T &v) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::find(first, last, v);
            }

            // Run find on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, v); });

            // Run find on lhs: [first, middle]
            I lhs = operator()(ex, p, first, middle, v);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                rhs.wait();
                if (lhs != middle) {
                    return lhs;
                } else {
                    return rhs.get();
                }
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                if (lhs != middle) {
                    return lhs;
                } else {
                    return operator()(make_inline_executor(), p, middle, last, v);
                }
            }
        }
    };

    /// \brief Finds the first element equal to another element
    inline constexpr find_functor find;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_FIND_H
