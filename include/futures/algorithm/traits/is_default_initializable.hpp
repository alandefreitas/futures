//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_DEFAULT_INITIALIZABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_DEFAULT_INITIALIZABLE_HPP

#include <futures/algorithm/traits/is_constructible_from.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 default_initializable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_default_initializable = __see_below__;
#else
    template <class T, class = void>
    struct is_default_initializable : std::false_type {};

    template <class T>
    struct is_default_initializable<
        T,
        std::void_t<
            // clang-format off
            std::enable_if_t<is_constructible_from_v<T>>,
            decltype(T{})
            // clang-format on
            >> : std::true_type {};
#endif
    template <class T>
    constexpr bool is_default_initializable_v = is_default_initializable<
        T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_DEFAULT_INITIALIZABLE_HPP
