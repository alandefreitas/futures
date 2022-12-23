//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_NEW_THREAD_EXECUTOR_HPP
#define FUTURES_EXECUTOR_NEW_THREAD_EXECUTOR_HPP

/**
 *  @file executor/new_thread_executor.hpp
 *  @brief New thread executor
 *
 *  This file defines the new thread executor, which creates a new thread
 *  every time a new task is launched. This is somewhat equivalent to executing
 *  tasks with C++11 `std::async`.
 */

#include <futures/config.hpp>
#include <futures/launch.hpp>
#include <futures/executor/inline_executor.hpp>
#include <futures/executor/is_executor.hpp>
#include <thread>

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

    /// An executor that runs anything in a new thread, like std::async does
    class new_thread_executor {
    public:
        constexpr new_thread_executor() = default;

        template <class F>
        void
        execute(F &&f) const {
            // Like `std::async`, this creates a detached thread. We can
            // ensure it won't be destructed before the task completes
            // because the future will take care of ensuring the task is
            // complete.
            std::thread(f).detach();
        }
    };

    /// Make an new thread executor object
    constexpr new_thread_executor
    make_new_thread_executor() {
        return {};
    }

    /** @} */ // @addtogroup executors Executors
} // namespace futures

#endif // FUTURES_EXECUTOR_NEW_THREAD_EXECUTOR_HPP
