
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_ALL_OF_H
#define FUTURES_ALL_OF_H

#include <execution>
#include <variant>

#include <futures/algorithm/detail/traits/range/range/concepts.hpp>

#include <futures/futures.h>
#include <futures/algorithm/algorithm_traits.h>
#include <futures/algorithm/partitioner.h>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// Class representing the overloads for the @ref all_of function
    class all_of_fn : public detail::unary_invoke_algorithm_fn<all_of_fn>
    {
      public:
        /// \brief Complete overload of the all_of algorithm
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
        /// \brief function template \c all_of
        template <class E, class P, class I, class S, class Fun
#ifndef FUTURES_DOXYGEN
                  ,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> && futures::detail::indirectly_unary_invocable<Fun, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0
#endif
                  >
        bool main(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::all_of(first, last, f);
            }

            // Run all_of on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, f); });

            // Run all_of on lhs: [first, middle]
            bool lhs = operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs && rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                if (!lhs) {
                    return false;
                } else {
                    return operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Checks if a predicate is true for all the elements in a range
    inline constexpr all_of_fn all_of;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_ALL_OF_H
