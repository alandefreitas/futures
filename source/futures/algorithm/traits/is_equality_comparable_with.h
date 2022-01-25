//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_WITH_H
#define FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_WITH_H

#include <futures/algorithm/traits/is_equality_comparable.h>
#include <futures/algorithm/traits/is_weakly_equality_comparable.h>
#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 equality_comparable_with
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T, class U>
    using is_equality_comparable_with = __see_below__;
#else
    template <class T, class U, class = void>
    struct is_equality_comparable_with : std::false_type
    {};

    template <class T, class U>
    struct is_equality_comparable_with<
        T,
        U,
        std::enable_if_t<
            // clang-format off
            is_equality_comparable_v<T> &&
            is_equality_comparable_v<U> &&
            is_weakly_equality_comparable_v<T,U>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T, class U>
    bool constexpr is_equality_comparable_with_v
        = is_equality_comparable_with<T, U>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_WITH_H
