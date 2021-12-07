//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_FOR_EACH_H
#define FUTURES_FOR_EACH_H

#include <execution>
#include <variant>

#include <futures/algorithm/detail/traits/range/range/concepts.hpp>

#include <futures/futures.h>
#include <futures/algorithm/algorithm_traits.h>
#include <futures/algorithm/detail/try_async.h>
#include <futures/algorithm/partitioner.h>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref for_each function
    class for_each_functor : public detail::unary_invoke_algorithm_functor<for_each_functor> {
      public:
        /// \brief Complete overload of the for_each algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param f Function
        /// \brief function template \c for_each
        template <class E, class P, class I, class S, class Fun,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> && futures::detail::indirectly_unary_invocable<Fun, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0>
        void main(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                std::for_each(first, last, f);
                return;
            }

            // Run for_each on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { operator()(ex, p, middle, last, f); });

            // Run for_each on lhs: [first, middle]
            operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                operator()(make_inline_executor(), p, middle, last, f);
            }
        }
    };

    /// \brief Applies a function to a range of elements
    inline constexpr for_each_functor for_each;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_FOR_EACH_H
