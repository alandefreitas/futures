//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_IS_READY_HPP
#define FUTURES_FUTURES_IS_READY_HPP

#include <futures/futures/basic_future.hpp>
#include <futures/futures/promise.hpp>
#include <futures/futures/traits/is_future.hpp>
#include <futures/detail/traits/has_is_ready.hpp>
#include <future>

namespace futures {
    /** @addtogroup adaptors Adaptors
     *  @{
     */

    /// Check if a future is ready
    ///
    /// Although basic_future has its more efficient is_ready function, this
    /// free function allows us to query other futures that don't implement
    /// is_ready, such as std::future.
    template <
        typename Future
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<is_future_v<std::decay_t<Future>>, int> = 0
#endif
        >
    bool
    is_ready(Future &&f) {
        assert(
            f.valid()
            && "Undefined behaviour. Checking if an invalid future is ready.");
        if constexpr (detail::has_is_ready_v<Future>) {
            return f.is_ready();
        } else {
            return f.wait_for(std::chrono::seconds(0))
                   == std::future_status::ready;
        }
    }
}

#endif // FUTURES_FUTURES_IS_READY_HPP
