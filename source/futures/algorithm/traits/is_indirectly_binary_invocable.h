//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_BINARY_INVOCABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_BINARY_INVOCABLE_H

#include <futures/algorithm/traits/is_indirectly_readable.h>
#include <futures/algorithm/traits/is_convertible_to.h>
#include <futures/algorithm/traits/iter_value.h>
#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 indirectly_binary_invocable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class F, class I1, class I2>
    using is_indirectly_binary_invocable = __see_below__;
#else
    template <class F, class I1, class I2, class = void>
    struct is_indirectly_binary_invocable : std::false_type
    {};

    template <class F, class I1, class I2>
    struct is_indirectly_binary_invocable<
        F, I1, I2,
        std::enable_if_t<
            // clang-format off
            is_indirectly_readable_v<I1> &&
            is_indirectly_readable_v<I2> &&
            std::is_copy_constructible_v<F> &&
            std::is_invocable_v<F&, iter_value_t<I1>&, iter_value_t<I2>&>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class F, class I1, class I2>
    bool constexpr is_indirectly_binary_invocable_v = is_indirectly_binary_invocable<F, I1, I2>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_BINARY_INVOCABLE_H
