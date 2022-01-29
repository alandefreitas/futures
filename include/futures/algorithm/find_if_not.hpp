//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FIND_IF_NOT_H
#define FUTURES_FIND_IF_NOT_H

#include <futures/algorithm/find_if.hpp>
#include <futures/algorithm/partitioner/partitioner.hpp>
#include <futures/algorithm/traits/unary_invoke_algorithm.hpp>
#include <futures/futures.hpp>
#include <execution>
#include <variant>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */
    /** \addtogroup functions Functions
     *  @{
     */


    /// \brief Functor representing the overloads for the @ref find_if_not
    /// function
    class find_if_not_functor
        : public unary_invoke_algorithm_functor<find_if_not_functor>
    {
        friend unary_invoke_algorithm_functor<find_if_not_functor>;

        /// \brief Complete overload of the find_if_not algorithm
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
        /// \brief function template \c find_if_not
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
        I
        run(const E& ex, P p, I first, S last, Fun f) const {
            return find_if_functor::find_if_graph<E, I>(ex)
                .find_if(p, first, last, [f](const auto& el) {
                    return !f(el);
                });
        }
    };

    /// \brief Finds the first element not satisfying specific criteria
    inline constexpr find_if_not_functor find_if_not;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_FIND_IF_NOT_H
