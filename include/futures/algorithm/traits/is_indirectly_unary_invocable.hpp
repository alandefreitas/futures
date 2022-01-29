//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_UNARY_INVOCABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_UNARY_INVOCABLE_H

#include <futures/algorithm/traits/is_indirectly_readable.hpp>
#include <futures/algorithm/traits/is_convertible_to.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 indirectly_unary_invocable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class F, class I>
    using is_indirectly_unary_invocable = __see_below__;
#else
    template <class F, class I, class = void>
    struct is_indirectly_unary_invocable : std::false_type
    {};

    template <class F, class I>
    struct is_indirectly_unary_invocable<
        F, I,
        std::enable_if_t<
            // clang-format off
            is_indirectly_readable_v<I> &&
            std::is_copy_constructible_v<F> &&
            std::is_invocable_v<F&, iter_value_t<I>&>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class F, class I>
    bool constexpr is_indirectly_unary_invocable_v = is_indirectly_unary_invocable<F, I>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_UNARY_INVOCABLE_H
