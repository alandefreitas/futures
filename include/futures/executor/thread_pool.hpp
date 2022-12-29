//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_THREAD_POOL_HPP
#define FUTURES_EXECUTOR_THREAD_POOL_HPP

/**
 *  @file executor/thread_pool.hpp
 *  @brief Inline executor
 *
 *  This file defines a thread pool and its executor.
 */

#include <futures/config.hpp>
#include <futures/executor/hardware_concurrency.hpp>
#include <futures/detail/deps/asio/thread_pool.hpp>

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

    /// A thread pool with the specified number of threads
    class thread_pool {
        futures::asio::thread_pool pool_;

    public:
        /// A executor that sends tasks to the thread pool
        class executor_type {
            friend class thread_pool;
            asio::thread_pool::executor_type ex_;
            executor_type(asio::thread_pool::executor_type ex) : ex_(ex) {}
        public:
            template <class F>
            void
            execute(F &&f) const {
                ex_.post(std::forward<F>(f), std::allocator<void>{});
            }
        };

        /// Construct a thread pool
        thread_pool() : thread_pool(hardware_concurrency()) {}

        /// Construct a thread pool with specified number of threads
        thread_pool(unsigned int threads) : pool_(threads) {}

        /// Thread pools cannot be copied
        thread_pool(thread_pool &) = delete;

        /// Thread pools cannot be moved
        thread_pool(thread_pool &&) = delete;

        executor_type
        get_executor() {
            return thread_pool::executor_type(pool_.get_executor());
        }

        void
        join() {
            return pool_.join();
        }
    };

    /** @} */ // @addtogroup executors Executors
} // namespace futures

#endif // FUTURES_EXECUTOR_THREAD_POOL_HPP
