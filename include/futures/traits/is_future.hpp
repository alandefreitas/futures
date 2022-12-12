//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_IS_FUTURE_HPP
#define FUTURES_TRAITS_IS_FUTURE_HPP

/**
 *  @file traits/is_future.hpp
 *  @brief `is_future` trait
 *
 *  This file defines the `is_future` trait.
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

    /// Customization point to determine if a type is a future type
    template <typename>
    struct is_future : std::false_type {};

    /// @copydoc is_future
    template <class T>
    struct is_future<std::future<T>> : std::true_type {};

    /// @copydoc is_future
    template <class T>
    struct is_future<std::shared_future<T>> : std::true_type {};

    /// @copydoc is_future
    template <class T>
    constexpr bool is_future_v = is_future<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_IS_FUTURE_HPP
