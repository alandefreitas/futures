//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_IS_CONTINUABLE_HPP
#define FUTURES_TRAITS_IS_CONTINUABLE_HPP

/**
 *  @file traits/is_continuable.hpp
 *  @brief `is_continuable` trait
 *
 *  This file defines the `is_continuable` trait.
 */

#include <futures/config.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *
     * \brief Determine properties of future types
     *
     *  @{
     */

    /// Customization point to define future as supporting lazy
    /// continuations
    template <typename>
    struct is_continuable : std::false_type {};

    /// Customization point to define future as supporting lazy
    /// continuations
    template <class T>
    constexpr bool is_continuable_v = is_continuable<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_IS_CONTINUABLE_HPP
