//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_PARTIALLY_ORDERED_WITH_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_PARTIALLY_ORDERED_WITH_HPP

#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 partially_ordered_with
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T, class U>
    using is_partially_ordered_with = __see_below__;
#else
    template <class T, class U, class = void>
    struct is_partially_ordered_with : std::false_type
    {};

    template <class T, class U>
    struct is_partially_ordered_with<
        T,
        U,
        std::void_t<
            // clang-format off
            decltype(std::declval<const std::remove_reference_t<T>&>() < std::declval<const std::remove_reference_t<U>&>()),
            decltype(std::declval<const std::remove_reference_t<T>&>() > std::declval<const std::remove_reference_t<U>&>()),
            decltype(std::declval<const std::remove_reference_t<T>&>() <= std::declval<const std::remove_reference_t<U>&>()),
            decltype(std::declval<const std::remove_reference_t<T>&>() >= std::declval<const std::remove_reference_t<U>&>()),
            decltype(std::declval<const std::remove_reference_t<U>&>() < std::declval<const std::remove_reference_t<T>&>()),
            decltype(std::declval<const std::remove_reference_t<U>&>() > std::declval<const std::remove_reference_t<T>&>()),
            decltype(std::declval<const std::remove_reference_t<U>&>() <= std::declval<const std::remove_reference_t<T>&>()),
            decltype(std::declval<const std::remove_reference_t<U>&>() >= std::declval<const std::remove_reference_t<T>&>())
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T, class U>
    bool constexpr is_partially_ordered_with_v
        = is_partially_ordered_with<T, U>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_PARTIALLY_ORDERED_WITH_HPP
