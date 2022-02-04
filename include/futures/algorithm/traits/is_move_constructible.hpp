//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP

#include <futures/algorithm/traits/is_constructible_from.hpp>
#include <futures/algorithm/traits/is_convertible_to.hpp>
#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 move_constructible
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_move_constructible = __see_below__;
#else
    template <class T, class = void>
    struct is_move_constructible : std::false_type
    {};

    template <class T>
    struct is_move_constructible<
        T,
        std::enable_if_t<
            // clang-format off
            is_constructible_from_v<T, T> &&
            is_convertible_to_v<T, T>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_move_constructible_v = is_move_constructible<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP
