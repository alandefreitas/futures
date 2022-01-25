//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_SUBTRACTABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_SUBTRACTABLE_H

#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 subtractable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_subtractable = __see_below__;
#else
    template <class T, class = void>
    struct is_subtractable : std::false_type
    {};

    template <class T>
    struct is_subtractable<
        T,
        std::void_t<
            // clang-format off
            decltype(std::declval<const std::remove_reference_t<T>&>() - std::declval<const std::remove_reference_t<T>&>())
            // clang-format on
            >>
        : std::is_integral<
              decltype(std::declval<const std::remove_reference_t<T>&>() - std::declval<const std::remove_reference_t<T>&>())>
    {};
#endif
    template <class T>
    bool constexpr is_subtractable_v = is_subtractable<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_SUBTRACTABLE_H
