//
// Created by Alan Freitas on 8/17/21.
//

// Don't let asio compile definitions at this point
#ifndef ASIO_SEPARATE_COMPILATION
#define ASIO_SEPARATE_COMPILATION
#endif
#include <futures/config/asio_include.h>
#include <futures/executor/default_executor.h>

namespace futures {
    asio::thread_pool &default_execution_context() {
        static asio::thread_pool pool(hardware_concurrency() * 3);
        return pool;
    }

    asio::thread_pool::executor_type make_default_executor() {
        asio::thread_pool &pool = default_execution_context();
        return pool.executor();
    }

    std::size_t hardware_concurrency() noexcept {
        // Cache the value because calculating it may be expensive
        static std::size_t value = std::thread::hardware_concurrency();

        // Always return at least 1 core
        return value == 0 ? 1 : value;
    }
} // namespace futures