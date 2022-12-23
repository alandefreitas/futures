//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_HPP

/**
 *  @file algorithm/traits/is_movable.hpp
 *  @brief `is_movable` trait
 *
 *  This file defines the `is_movable` trait.
 */

#include <futures/algorithm/traits/is_assignable_from.hpp>
#include <futures/algorithm/traits/is_move_constructible.hpp>
#include <futures/algorithm/traits/is_swappable.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::movable` concept
    /**
     * @see https://en.cppreference.com/w/cpp/concepts/movable
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_movable = std::bool_constant<std::movable<T>>;
#else
    template <class T>
    using is_movable = detail::conjunction<
        std::is_object<T>,
        is_move_constructible<T>,
        is_assignable_from<T&, T>,
        is_swappable<T>>;
#endif

    /// @copydoc is_movable
    template <class T>
    constexpr bool is_movable_v = is_movable<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_HPP
