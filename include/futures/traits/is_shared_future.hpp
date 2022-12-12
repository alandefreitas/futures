//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_IS_SHARED_FUTURE_HPP
#define FUTURES_TRAITS_IS_SHARED_FUTURE_HPP

/**
 *  @file traits/is_shared_future.hpp
 *  @brief `is_shared_future` trait
 *
 *  This file defines the `is_shared_future` trait.
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

    /// Customization point to determine if a type is a shared future type
    template <typename>
    struct is_shared_future : std::false_type {};

    /// @copydoc is_shared_future
    template <class T>
    constexpr bool is_shared_future_v = is_shared_future<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_IS_SHARED_FUTURE_HPP
