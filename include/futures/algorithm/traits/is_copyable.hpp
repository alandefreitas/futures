//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_COPYABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_COPYABLE_HPP

#include <futures/algorithm/traits/is_assignable_from.hpp>
#include <futures/algorithm/traits/is_movable.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 copyable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_copyable = __see_below__;
#else
    template <class T, class = void>
    struct is_copyable : std::false_type
    {};

    template <class T>
    struct is_copyable<
        T,
        std::enable_if_t<
            // clang-format off
            std::is_copy_constructible_v<T> &&
            is_movable_v<T> &&
            is_assignable_from_v<T&, T&> &&
            is_assignable_from_v<T&, const T&> &&
            is_assignable_from_v<T&, const T>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_copyable_v = is_copyable<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_COPYABLE_HPP
