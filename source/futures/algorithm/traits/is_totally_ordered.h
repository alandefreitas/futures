//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_TOTALLY_ORDERED_H
#define FUTURES_ALGORITHM_TRAITS_IS_TOTALLY_ORDERED_H

#include <type_traits>
#include <futures/algorithm/traits/is_equality_comparable.h>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 totally_ordered
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_totally_ordered = __see_below__;
#else
    template <class T, class = void>
    struct is_totally_ordered : std::false_type
    {};

    template <class T>
    struct is_totally_ordered<
        T,
        std::void_t<
            // clang-format off
            decltype(std::declval<const std::remove_reference_t<T>&>() < std::declval<const std::remove_reference_t<T>&>()),
            decltype(std::declval<const std::remove_reference_t<T>&>() > std::declval<const std::remove_reference_t<T>&>()),
            decltype(std::declval<const std::remove_reference_t<T>&>() <= std::declval<const std::remove_reference_t<T>&>()),
            decltype(std::declval<const std::remove_reference_t<T>&>() >= std::declval<const std::remove_reference_t<T>&>())
            // clang-format on
            >> : is_equality_comparable<T>
    {};
#endif
    template <class T>
    bool constexpr is_totally_ordered_v = is_totally_ordered<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_TOTALLY_ORDERED_H
