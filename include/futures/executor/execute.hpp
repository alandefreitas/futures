//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_EXECUTE_HPP
#define FUTURES_EXECUTOR_EXECUTE_HPP

#include <futures/config.hpp>
#include <futures/executor/is_execution_context.hpp>
#include <futures/executor/is_executor.hpp>
#include <futures/executor/detail/execute.hpp>
#include <type_traits>

namespace futures {
    /// Submits a task for execution
    /**
     * This free function submits a task for execution using the specified
     * executor.
     *
     * Unlike the `execute` member function of executors, this function
     * identifies and interoperates with other executor types, such as Asio
     * executors. If an execution context is provided, its executor is
     * retrived and used instead.
     *
     * @param ex The target executor
     * @param f The task
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <class F, executor_for<F> E>
#else
    template <
        class E,
        class F,
        std::enable_if_t<is_executor_for<E, F>::value, int> = 0>
#endif
    void
    execute(E const& ex, F&& f) {
        detail::execute_in_executor(ex, std::forward<F>(f));
    }

    /// Submits a task for execution on an execution context
    /**
     * This free function submits a task for execution using the specified
     * execution context.
     *
     * This is a convenience function that extracts the executor from the
     * context and uses it instead.
     *
     * @param ctx The target execution context
     * @param f The task
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <class F, execution_context_for<F> C>
    requires(!executor_for<C, F>)
#else
    template <
        class C,
        class F,
        std::enable_if_t<
            (!is_executor_for<C, F>::value
             && is_execution_context_for_v<C, F>),
            int>
        = 0>
#endif
    void execute(C& ctx, F&& f) {
        detail::execute_in_context(ctx, std::forward<F>(f));
    }
} // namespace futures

#endif // FUTURES_EXECUTOR_EXECUTE_HPP
