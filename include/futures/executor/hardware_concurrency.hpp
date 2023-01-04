//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_HARDWARE_CONCURRENCY_HPP
#define FUTURES_EXECUTOR_HARDWARE_CONCURRENCY_HPP

/**
 *  @file executor/hardware_concurrency.hpp
 *  @brief Hardware concurrency function
 *
 *  This file defines the hardware_concurrency used in futures.
 */

#include <futures/config.hpp>
#include <futures/detail/utility/is_constant_evaluated.hpp>
#include <thread>

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

    /// A version of hardware_concurrency that always returns at least 1
    /**
     *  This function is a safer version of hardware_concurrency that always
     *  returns at least 1 to represent the current context when the value is
     *  not computable.
     *
     *  - It never returns 0, 1 is returned instead.
     *  - It is guaranteed to remain constant for the duration of the program.
     *
     *  It also improves on hardware_concurrency to provide a default value
     *  of 1 when the function is being executed at compile time. This allows
     *  partitioners and algorithms to be constexpr.
     *
     *  @see
     *  [`std::hardware_concurrency`](https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency)
     *
     *  @return Number of concurrent threads supported. If the value is not
     *  well-defined or not computable, returns 1.
     **/
    FUTURES_CONSTANT_EVALUATED_CONSTEXPR_DECLARE unsigned int
    hardware_concurrency() noexcept {
        if (detail::is_constant_evaluated()) {
            return 1;
        } else {
            unsigned int value = std::thread::hardware_concurrency();
            if (value > 0) {
                return value;
            }
            return 1;
        }
    }

    /** @} */ // @addtogroup executors Executors
} // namespace futures

#endif // FUTURES_EXECUTOR_HARDWARE_CONCURRENCY_HPP
