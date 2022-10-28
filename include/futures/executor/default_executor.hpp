//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_DEFAULT_EXECUTOR_HPP
#define FUTURES_EXECUTOR_DEFAULT_EXECUTOR_HPP

/**
 *  @file executor/default_executor.hpp
 *  @brief Default executor and related functions
 *
 *  This file defines the default executor and related functions.
 *  The default executor is a dynamic thread pool.
 */

#include <futures/config.hpp>
#include <futures/executor/is_executor.hpp>
#include <futures/detail/utility/is_constant_evaluated.hpp>
#include <futures/detail/deps/asio/thread_pool.hpp>
#include <thread>

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

    /// The default execution context for async operations
    /**
     *  Unless an executor is explicitly provided, this is the executor we use
     *  for async operations.
     *
     *  This is the ASIO thread pool execution context with a default number of
     *  threads. However, the default execution context (and its type) might
     *  change in other versions of this library if something more general comes
     *  along. As the standard for executors gets adopted, libraries are likely
     *  to provide better implementations.
     *
     *  Also note that executors might not allow work-stealing. This needs to be
     *  taken into account when implementing algorithms with recursive tasks.
     *  One common options is to use `try_async` for recursive tasks.
     *
     *  Also note that, in the executors notation, the pool is an execution
     *  context but not an executor:
     *  - Execution context: a place where we can execute functions
     *  - A thread pool is an execution context, not an executor
     *
     *  An execution context is:
     *  - Usually long lived
     *  - Non-copyable
     *  - May contain additional state, such as timers, and threads
     **/
    using default_execution_context_type =
#ifndef FUTURES_DOXYGEN
        asio::thread_pool;
#else
        __implementation_defined__;
#endif

    /// Default executor type
    using default_executor_type = default_execution_context_type::executor_type;

    /// Create an instance of the default execution context
    ///
    /// @return Reference to the default execution context for @ref async
    FUTURES_DECLARE default_execution_context_type&
    default_execution_context();

    /// Create an Asio thread pool executor for the default thread pool
    /**
     * In the executors notation:
     * - Executor: set of rules governing where, when and how to run a function
     * object
     *   - A thread pool is an execution context for which we can create
     *   executors pointing to the pool.
     *   - The executor rule for the default thread pool executor is to run
     *   function objects in the pool
     *     and nowhere else.
     *
     * An executor is:
     * - Lightweight and copyable (just references and pointers to the
     * execution context).
     * - May be long or short lived.
     * - May be customized on a fine-grained basis, such as exception behavior,
     * and order
     *
     * There might be many executor types associated with with the same
     * execution context.
     *
     * @return Executor handle to the default execution context
     **/
    FUTURES_DECLARE default_execution_context_type::executor_type
    make_default_executor();

    /** @} */ // @addtogroup executors Executors
} // namespace futures

#ifdef FUTURES_HEADER_ONLY
#    include <futures/executor/impl/default_executor.ipp>
#endif

#endif // FUTURES_EXECUTOR_DEFAULT_EXECUTOR_HPP
