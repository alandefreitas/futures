//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_REGULAR_H
#define FUTURES_ALGORITHM_TRAITS_IS_REGULAR_H

#include <futures/algorithm/traits/is_equality_comparable.h>
#include <futures/algorithm/traits/is_semiregular.h>
#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 regular
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_regular = __see_below__;
#else
    template <class T, class = void>
    struct is_regular : std::false_type
    {};

    template <class T>
    struct is_regular<
        T,
        std::enable_if_t<
            // clang-format off
            is_semiregular_v<T> &&
            is_equality_comparable_v<T>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_regular_v = is_regular<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_REGULAR_H
