//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_HAS_STOP_TOKEN_HPP
#define FUTURES_TRAITS_HAS_STOP_TOKEN_HPP

/**
 *  @file traits/has_stop_token.hpp
 *  @brief `has_stop_token` trait
 *
 *  This file defines the `has_stop_token` trait.
 */

#include <future>
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

    /// Customization point to define future as having a common stop token
    /**
     * Besides being stoppable, this trait identifies whether the future
     * has a stop token, which means this token can be shared with other
     * futures to create a common thread of futures that can be stopped
     * with the same token.
     *
     * @see
     *      @li @ref is_stoppable
     */
    template <typename>
    struct has_stop_token : std::false_type {};

    /// @copydoc has_stop_token
    template <class T>
    constexpr bool has_stop_token_v = has_stop_token<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_HAS_STOP_TOKEN_HPP
