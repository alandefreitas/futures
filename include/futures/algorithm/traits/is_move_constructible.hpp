//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP

/**
 *  @file algorithm/traits/is_move_constructible.hpp
 *  @brief `is_move_constructible` trait
 *
 *  This file defines the `is_move_constructible` trait.
 */

#include <futures/algorithm/traits/is_constructible_from.hpp>
#include <futures/algorithm/traits/is_convertible_to.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /// @brief A type trait equivalent to the `std::move_constructible` concept
    /**
     * @see https://en.cppreference.com/w/cpp/concepts/move_constructible
     */
#if defined(FUTURES_DOXYGEN)
    template <class T>
    using is_move_constructible = std::bool_constant<std::move_constructible<T>>;
#else
    template <class T>
    using is_move_constructible = detail::
        conjunction<is_constructible_from<T, T>, is_convertible_to<T, T>>;
#endif

    /// @copydoc is_move_constructible
    template <class T>
    constexpr bool is_move_constructible_v = is_move_constructible<T>::value;

    /** @} */
    /** @} */

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP
