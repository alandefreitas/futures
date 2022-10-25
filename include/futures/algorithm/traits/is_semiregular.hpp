//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_SEMIREGULAR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_SEMIREGULAR_HPP

/**
 *  @file algorithm/traits/is_semiregular.hpp
 *  @brief `is_semiregular` trait
 *
 *  This file defines the `is_semiregular` trait.
 */

#include <futures/algorithm/traits/is_copyable.hpp>
#include <futures/algorithm/traits/is_default_initializable.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::semiregular` concept
    /**
     * @see https://en.cppreference.com/w/cpp/concepts/semiregular
     */
    template <class T>
    using is_semiregular = std::
        conjunction<is_copyable<T>, is_default_initializable<T>>;


    /// @copydoc is_semiregular
    template <class T>
    constexpr bool is_semiregular_v = is_semiregular<T>::value;

    /** @} */
    /** @} */

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_SEMIREGULAR_HPP
