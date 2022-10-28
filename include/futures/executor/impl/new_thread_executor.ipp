//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_IMPL_NEW_THREAD_EXECUTOR_IPP
#define FUTURES_EXECUTOR_IMPL_NEW_THREAD_EXECUTOR_IPP

/**
 *  @file executor/new_thread_executor.hpp
 *  @brief New thread executor
 *
 *  This file defines the new thread executor, which creates a new thread
 *  every time a new task is launched. This is somewhat equivalent to executing
 *  tasks with C++11 `std::async`.
 */

#include <futures/executor/new_thread_executor.hpp>

namespace futures {
    new_thread_executor
    make_new_thread_executor() {
        asio::execution_context &ctx = inline_execution_context();
        return new_thread_executor{ &ctx };
    }
} // namespace futures

#endif // FUTURES_EXECUTOR_IMPL_NEW_THREAD_EXECUTOR_IPP
