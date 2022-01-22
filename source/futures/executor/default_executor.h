//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DEFAULT_EXECUTOR_H
#define FUTURES_DEFAULT_EXECUTOR_H

#include <futures/config/asio_include.h>
#include <futures/executor/is_executor.h>

namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

    /// \brief A version of hardware_concurrency that always returns at least 1
    ///
    /// This function is a safer version of hardware_concurrency that always
    /// returns at least 1 to represent the current context when the value is
    /// not computable.
    ///
    /// - It never returns 0, 1 is returned instead.
    /// - It is guaranteed to remain constant for the duration of the program.
    ///
    /// \see
    /// https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency
    ///
    /// \return Number of concurrent threads supported. If the value is not
    /// well-defined or not computable, returns 1.
    std::size_t
    hardware_concurrency() noexcept;
    inline std::size_t
    hardware_concurrency() noexcept {
        // Cache the value because calculating it may be expensive
        static std::size_t value = std::thread::hardware_concurrency();

        // Always return at least 1 core
        return std::max(static_cast<std::size_t>(1), value);
    }

    /// \brief The default execution context for async operations, unless
    /// otherwise stated
    ///
    /// Unless an executor is explicitly provided, this is the executor we use
    /// for async operations.
    ///
    /// This is the ASIO thread pool execution context with a default number of
    /// threads. However, the default execution context (and its type) might
    /// change in other versions of this library if something more general comes
    /// along. As the standard for executors gets adopted, libraries are likely
    /// to provide better implementations.
    ///
    /// Also note that executors might not allow work-stealing. This needs to be
    /// taken into account when implementing algorithms with recursive tasks.
    /// One common options is to use `try_async` for recursive tasks.
    ///
    /// Also note that, in the executors notation, the pool is an execution
    /// context but not an executor:
    /// - Execution context: a place where we can execute functions
    /// - A thread pool is an execution context, not an executor
    ///
    /// An execution context is:
    /// - Usually long lived
    /// - Non-copyable
    /// - May contain additional state, such as timers, and threads
    ///
#ifndef FUTURES_DOXYGEN
    using default_execution_context_type = asio::thread_pool;
#else
    using default_execution_context_type = __implementation_defined__;
#endif

    /// \brief Default executor type as a constant trait for future_base
    /// functions
    using default_executor_type = default_execution_context_type::executor_type;

    /// \brief Create an instance of the default execution context
    ///
    /// \return Reference to the default execution context for @ref async
    inline default_execution_context_type &
    default_execution_context() {
#ifdef FUTURES_DEFAULT_THREAD_POOL_SIZE
        const std::size_t default_thread_pool_size
            = FUTURES_DEFAULT_THREAD_POOL_SIZE;
#else
        const std::size_t default_thread_pool_size = hardware_concurrency();
#endif
        static asio::thread_pool pool(default_thread_pool_size);
        return pool;
    }

    /// \brief Create an Asio thread pool executor for the default thread pool
    ///
    /// In the executors notation:
    /// - Executor: set of rules governing where, when and how to run a function
    /// object
    ///   - A thread pool is an execution context for which we can create
    ///   executors pointing to the pool.
    ///   - The executor rule for the default thread pool executor is to run
    ///   function objects in the pool
    ///     and nowhere else.
    ///
    /// An executor is:
    /// - Lightweight and copyable (just references and pointers to the
    /// execution context).
    /// - May be long or short lived.
    /// - May be customized on a fine-grained basis, such as exception behavior,
    /// and order
    ///
    /// There might be many executor types associated with with the same
    /// execution context.
    ///
    /// \return Executor handle to the default execution context
    inline default_execution_context_type::executor_type
    make_default_executor() {
        asio::thread_pool &pool = default_execution_context();
        return pool.executor();
    }

    /** @} */ // \addtogroup executors Executors
} // namespace futures

#endif // FUTURES_DEFAULT_EXECUTOR_H
