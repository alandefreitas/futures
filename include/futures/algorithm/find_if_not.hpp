//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_FIND_IF_NOT_HPP
#define FUTURES_ALGORITHM_FIND_IF_NOT_HPP

#include <futures/algorithm/find_if.hpp>
#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/traits/unary_invoke_algorithm.hpp>
#include <futures/futures.hpp>
#include <execution>
#include <variant>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */
    /** @addtogroup functions Functions
     *  @{
     */

    /// Functor representing the overloads for the @ref find_if_not
    /// function
    class find_if_not_functor
        : public unary_invoke_algorithm_functor<find_if_not_functor>
    {
        friend unary_invoke_algorithm_functor<find_if_not_functor>;

        template <
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        static FUTURES_CONSTANT_EVALUATED_CONSTEXPR I
        inline_find_if_not(I first, S last, Fun p) {
            for (; first != last; ++first) {
                if (!p(*first)) {
                    return first;
                }
            }
            return last;
        }

        /// Complete overload of the find_if_not algorithm
        /// @tparam E Executor type
        /// @tparam P Partitioner type
        /// @tparam I Iterator type
        /// @tparam S Sentinel iterator type
        /// @tparam Fun Function type
        /// @param ex Executor
        /// @param p Partitioner
        /// @param first Iterator to first element in the range
        /// @param last Iterator to (last + 1)-th element in the range
        /// @param f Function
        /// function template \c find_if_not
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        FUTURES_CONSTANT_EVALUATED_CONSTEXPR I
        run(const E& ex, P p, I first, S last, Fun f) const {
            if constexpr (std::is_same_v<std::decay_t<E>, inline_executor>) {
                return inline_find_if_not(first, last, f);
            } else {
                if (detail::is_constant_evaluated()) {
                    return inline_find_if_not(first, last, f);
                } else {
                    return find_if_functor::find_if_graph<E, I>(ex)
                        .find_if(p, first, last, [f](const auto& el) {
                            return !f(el);
                        });
                }
            }
        }
    };

    /// Finds the first element not satisfying specific criteria
    inline constexpr find_if_not_functor find_if_not;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_FIND_IF_NOT_HPP
