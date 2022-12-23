//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_COPYABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_COPYABLE_HPP

/**
 *  @file algorithm/traits/is_copyable.hpp
 *  @brief `is_copyable` trait
 *
 *  This file defines the `is_copyable` trait.
 */

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


    /// @brief A type trait equivalent to the `std::copyable` concept
    /**
     * @see https://en.cppreference.com/w/cpp/concepts/copyable
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_copyable = std::bool_constant<std::copyable<T>>;
#else
    template <class T>
    using is_copyable = detail::conjunction<
        std::is_copy_constructible<T>,
        is_movable<T>,
        is_assignable_from<T&, T&>,
        is_assignable_from<T&, std::add_const_t<T>&>,
        is_assignable_from<T&, std::add_const_t<T>>>;
#endif

    /// @copydoc is_copyable
    template <class T>
    constexpr bool is_copyable_v = is_copyable<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_COPYABLE_HPP
