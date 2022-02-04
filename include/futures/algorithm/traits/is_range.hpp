//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_RANGE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_RANGE_HPP

#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 range concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_range = __see_below__;
#else
    template <class T, class = void>
    struct is_range : std::false_type
    {};

    template <class T>
    struct is_range<
        T,
        std::void_t<
            // clang-format off
            decltype(*begin(std::declval<T>())),
            decltype(*end(std::declval<T>()))
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_range_v = is_range<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_RANGE_HPP
