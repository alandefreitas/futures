//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FIND_H
#define FUTURES_FIND_H

#include <futures/algorithm/comparisons/equal_to.h>
#include <futures/algorithm/partitioner/partitioner.h>
#include <futures/algorithm/traits/is_indirectly_binary_invocable.h>
#include <futures/algorithm/traits/value_cmp_algorithm.h>
#include <futures/futures.h>
#include <futures/algorithm/detail/try_async.h>
#include <execution>
#include <variant>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup functions Functions
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref find function
    class find_functor : public value_cmp_algorithm_functor<find_functor>
    {
        friend value_cmp_algorithm_functor<find_functor>;

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
        template <
            class E,
            class P,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<equal_to, T *, I>
                // clang-format on
                ,
                int> = 0
#endif
            >
        I
        run(const E &ex, P p, I first, S last, const T &v) const {
            auto middle = p(first, last);
            if (middle == last
                || std::is_same_v<
                    E,
                    inline_executor> || is_forward_iterator_v<I>)
            {
                return std::find(first, last, v);
            }

            // Run find on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel]
                = try_async(ex, [ex, p, middle, last, v, this]() {
                      return operator()(ex, p, middle, last, v);
                  });

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
                    return
                    operator()(make_inline_executor(), p, middle, last, v);
                }
            }
        }
    };

    /// \brief Finds the first element equal to another element
    inline constexpr find_functor find;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_FIND_H
