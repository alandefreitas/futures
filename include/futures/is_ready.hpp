//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IS_READY_HPP
#define FUTURES_IS_READY_HPP

/**
 *  @file is_ready.hpp
 *  @brief Free functions to check whether a future is ready
 *
 *  This file defines free functions to check whether any future type is
 *  ready. This is particularly useful for future types that do not offer the
 *  `is_ready()` member function.
 */

#include <futures/future.hpp>
#include <futures/promise.hpp>
#include <futures/traits/is_future.hpp>
#include <futures/detail/is_ready.hpp>
#include <future>

namespace futures {
    /** @addtogroup adaptors Adaptors
     *  @{
     */

    /// Check if a future is ready
    /**
     *  Although basic_future has its more efficient is_ready function, this
     *  free function allows us to query other futures that don't implement
     *  is_ready, such as std::future.
     */
    FUTURES_TEMPLATE(class Future)
    (requires is_future_v<std::decay_t<Future>>) bool is_ready(Future &&f);

    /**
     * @}
     */
} // namespace futures

#include <futures/impl/is_ready.hpp>

#endif // FUTURES_IS_READY_HPP
