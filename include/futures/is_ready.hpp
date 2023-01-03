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

#include <futures/config.hpp>
#include <futures/future.hpp>
#include <futures/promise.hpp>
#include <futures/traits/is_future_like.hpp>
#include <futures/detail/is_ready.hpp>

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
#ifdef FUTURES_HAS_CONCEPTS
    template <future_like Future>
#else
    template <
        class Future,
        std::enable_if_t<is_future_like_v<std::decay_t<Future>>, int> = 0>
#endif
    bool
    is_ready(Future &&f);

    /**
     * @}
     */
} // namespace futures

#include <futures/impl/is_ready.hpp>

#endif // FUTURES_IS_READY_HPP
