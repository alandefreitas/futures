//
// Created by Alan Freitas on 8/16/21.
//

#ifndef CPP_MANIFEST_FIND_H
#define CPP_MANIFEST_FIND_H

#include <execution>
#include <variant>

#include <range/v3/all.hpp>

#include "../futures.h"
#include "algorithm_traits.h"
#include "partitioner.h"

namespace futures {
    /// Class representing the overloads for the @ref find function
    class find_fn : public detail::value_cmp_algorithm_fn<find_fn> {
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
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && ranges::input_iterator<I> &&
                                       ranges::sentinel_for<S, I> &&
                                       ranges::indirectly_binary_invocable_<ranges::equal_to, T *, I>,
                                   int> = 0>
        I main(const E &ex, P p, I first, S last, const T &v) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || ranges::forward_iterator<I>) {
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
    inline constexpr find_fn find;

} // namespace futures

#endif // CPP_MANIFEST_FIND_H
