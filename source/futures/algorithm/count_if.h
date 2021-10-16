//
// Created by Alan Freitas on 8/16/21.
//

#ifndef CPP_MANIFEST_COUNT_IF_H
#define CPP_MANIFEST_COUNT_IF_H

#include <execution>
#include <variant>

#include <range/v3/all.hpp>

#include "../futures.h"
#include "algorithm_traits.h"
#include "partitioner.h"

namespace futures {
    /// Class representing the overloads for the @ref count_if function
    class count_if_fn : public detail::unary_invoke_algorithm_fn<count_if_fn> {
      public:
        /// \brief Complete overload of the count_if algorithm
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
        /// \brief function template \c count_if
        template <class E, class P, class I, class S, class Fun,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && ranges::input_iterator<I> &&
                                       ranges::sentinel_for<S, I> && ranges::indirectly_unary_invocable<Fun, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0>
        ranges::iter_difference_t<I> main(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || ranges::forward_iterator<I>) {
                return std::count_if(first, last, f);
            }

            // Run count_if on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, f); });

            // Run count_if on lhs: [first, middle]
            bool lhs = operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs + rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                return lhs + operator()(make_inline_executor(), p, middle, last, f);
            }
        }
    };

    /// \brief Returns the number of elements satisfying specific criteria
    inline constexpr count_if_fn count_if;

} // namespace futures

#endif // CPP_MANIFEST_COUNT_IF_H
