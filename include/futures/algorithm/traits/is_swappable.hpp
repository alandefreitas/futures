//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_SWAPPABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_SWAPPABLE_HPP

/**
 *  @file algorithm/traits/is_swappable.hpp
 *  @brief `is_swappable` trait
 *
 *  This file defines the `is_swappable` trait.
 */

#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::swappable`
    /// concept
    /**
     * @see https://en.cppreference.com/w/cpp/concepts/swappable
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_swappable = __see_below__;
#else
    template <class T, class = void>
    struct is_swappable : std::false_type {};

    template <class T>
    struct is_swappable<
        T,
        std::void_t<
            // clang-format off
            decltype(std::swap(std::declval<T&>(), std::declval<T&>()))
            // clang-format on
            >> : std::true_type {};
#endif

    /// @copydoc is_swappable
    template <class T>
    constexpr bool is_swappable_v = is_swappable<T>::value;

    /** @} */
    /** @} */

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_SWAPPABLE_HPP
