//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_IMPL_DEFAULT_EXECUTOR_IPP
#define FUTURES_EXECUTOR_IMPL_DEFAULT_EXECUTOR_IPP

#include <futures/executor/default_executor.hpp>
#include <futures/executor/hardware_concurrency.hpp>

namespace futures {
    default_execution_context_type &
    default_execution_context() {
#ifdef FUTURES_DEFAULT_THREAD_POOL_SIZE
        const std::size_t default_thread_pool_size
            = FUTURES_DEFAULT_THREAD_POOL_SIZE;
#else
        const std::size_t default_thread_pool_size = std::
            max(hardware_concurrency(), std::size_t(2));
#endif
        static asio::thread_pool pool(default_thread_pool_size);
        return pool;
    }

    default_execution_context_type::executor_type
    make_default_executor() {
        asio::thread_pool &pool = default_execution_context();
        return pool.executor();
    }
} // namespace futures

#endif // FUTURES_EXECUTOR_IMPL_DEFAULT_EXECUTOR_IPP
