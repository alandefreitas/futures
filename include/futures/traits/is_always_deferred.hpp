//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_IS_ALWAYS_DEFERRED_HPP
#define FUTURES_TRAITS_IS_ALWAYS_DEFERRED_HPP

/**
 *  @file traits/is_always_deferred.hpp
 *  @brief `is_always_deferred` trait
 *
 *  This file defines the `is_always_deferred` trait.
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

    /// Customization point to define a future as always deferred
    /**
     * Deferred futures allow optimizations that make it worth indicating
     * at compile time whether they can be applied. They can carry their
     * tasks, avoid dynamic memory allocations, and attach continuations
     * without any extra synchronization cost.
     *
     * Unless this trait is specialized, a type is considered to not be
     * always deferred.
     */
    template <typename>
    struct is_always_deferred : std::false_type {};

    /// Customization point to define future as always deferred
    template <class T>
    constexpr bool is_always_deferred_v = is_always_deferred<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_IS_ALWAYS_DEFERRED_HPP
