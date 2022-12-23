//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_FIND_IF_NOT_HPP
#define FUTURES_ALGORITHM_FIND_IF_NOT_HPP

/**
 *  @file algorithm/find_if_not.hpp
 *  @brief `find_if_not` algorithm
 *
 *  This file defines the functor and callable for a parallel version of the
 *  `find_if_not` algorithm.
 */

#include <futures/algorithm/find_if.hpp>
#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/traits/unary_invoke_algorithm.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/algorithm/detail/execution.hpp>
#include <futures/detail/deps/boost/core/ignore_unused.hpp>


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
        : public unary_invoke_algorithm_functor<find_if_not_functor> {
        friend unary_invoke_algorithm_functor<find_if_not_functor>;

        FUTURES_TEMPLATE(class I, class S, class Fun)
        (requires is_input_iterator_v<I>&& is_sentinel_for_v<S, I>&&
             is_indirectly_unary_invocable_v<Fun, I>&&
                 detail::is_copy_constructible_v<
                     Fun>) static FUTURES_CONSTANT_EVALUATED_CONSTEXPR I
            inline_find_if_not(I first, S last, Fun p) {
            for (; first != last; ++first) {
                if (!p(*first)) {
                    return first;
                }
            }
            return last;
        }

        /// Complete overload of the find_if_not algorithm
        /**
         *  @tparam E Executor type
         *  @tparam P Partitioner type
         *  @tparam I Iterator type
         *  @tparam S Sentinel iterator type
         *  @tparam Fun Function type
         *  @param ex Executor
         *  @param p Partitioner
         *  @param first Iterator to first element in the range
         *  @param last Iterator to (last + 1)-th element in the range
         *  @param f Function
         *  function template \c find_if_not
         */
        FUTURES_TEMPLATE(class E, class P, class I, class S, class Fun)
        (requires is_executor_v<E>&& is_partitioner_v<P, I, S>&&
             is_input_iterator_v<I>&& is_sentinel_for_v<S, I>&&
                 is_indirectly_unary_invocable_v<Fun, I>&&
                     detail::is_copy_constructible_v<Fun>)
            FUTURES_CONSTANT_EVALUATED_CONSTEXPR I
            run(E const& ex, P p, I first, S last, Fun f) const {
            FUTURES_IF_CONSTEXPR (
                detail::is_same_v<std::decay_t<E>, inline_executor>)
            {
                boost::ignore_unused(p);
                return inline_find_if_not(first, last, f);
            } else {
                if (detail::is_constant_evaluated()) {
                    boost::ignore_unused(p);
                    return inline_find_if_not(first, last, f);
                } else {
                    return find_if_functor::find_if_graph<E, I>(ex)
                        .find_if(p, first, last, [f](auto const& el) {
                            return !f(el);
                        });
                }
            }
        }
    };

    /// Finds the first element not satisfying specific criteria
    FUTURES_INLINE_VAR constexpr find_if_not_functor find_if_not;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_FIND_IF_NOT_HPP
