//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_HPP

#include <futures/algorithm/traits/is_move_constructible.hpp>
#include <futures/algorithm/traits/is_assignable_from.hpp>
#include <futures/algorithm/traits/is_swappable.hpp>
#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 movable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_movable = __see_below__;
#else
    template <class T, class = void>
    struct is_movable : std::false_type
    {};

    template <class T>
    struct is_movable<
        T,
        std::enable_if_t<
            // clang-format off
            std::is_object_v<T> &&
            is_move_constructible_v<T> &&
            is_assignable_from_v<T&, T> &&
            is_swappable_v<T>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_movable_v = is_movable<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_HPP
