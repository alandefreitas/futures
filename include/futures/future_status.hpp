//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURE_STATUS_HPP
#define FUTURES_FUTURE_STATUS_HPP

/**
 *  @file future_status.hpp
 *  @brief Future types
 *
 *  Define future status enumeration
 */

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */

    /** @addtogroup future-types Future types
     *  @{
     */

    /// Specifies state of a future
    /**
     * Specifies state of a future as returned by @ref wait_for and
     * @ref wait_until functions of @ref basic_future.
     */
    enum class future_status
    {
        /// The operation state is ready
        ready,
        /// The operation state did not become ready before specified timeout
        /// duration has passed
        timeout,
        /// The operation state contains a deferred function, so the result will
        /// be computed only when explicitly requested
        deferred
    };

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURE_STATUS_HPP
