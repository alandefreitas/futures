//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_REGULAR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_REGULAR_HPP

/**
 *  @file algorithm/traits/is_regular.hpp
 *  @brief `is_regular` trait
 *
 *  This file defines the `is_regular` trait.
 */

#include <futures/algorithm/traits/is_equality_comparable.hpp>
#include <futures/algorithm/traits/is_semiregular.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::regular` concept
    /**
     * @see https://en.cppreference.com/w/cpp/concepts/regular
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_regular = std::bool_constant<std::regular<T>>;
#else
    template <class T>
    using is_regular = detail::
        conjunction<is_semiregular<T>, is_equality_comparable<T>>;
#endif


    /// @copydoc is_regular
    template <class T>
    constexpr bool is_regular_v = is_regular<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_REGULAR_HPP
