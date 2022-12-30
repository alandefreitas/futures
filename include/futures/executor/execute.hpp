//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_EXECUTE_HPP
#define FUTURES_EXECUTOR_EXECUTE_HPP

#include <futures/config.hpp>
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
    template <class E, class F>
    requires(is_executor<E>::value || detail::is_asio_executor<E>::value)
#else
    template <
            class E,
            class F,
            std::enable_if_t<
                (is_executor<E>::value || detail::is_asio_executor<E>::value),
                int>
            = 0>
#endif
    void execute(E const& ex, F&& f) {
        detail::execute_in_executor(ex, std::forward<F>(f));
    }

    /// @copydoc execute
#ifdef FUTURES_HAS_CONCEPTS
    template <class E, class F>
    requires(
        !is_executor<E>::value && !detail::is_asio_executor<E>::value
        && detail::has_get_executor<E>::value)
#else
    template <class E, class F, std::enable_if_t<(
        !is_executor<E>::value && !detail::is_asio_executor<E>::value
        && detail::has_get_executor<E>::value), int> = 0>
#endif
    void execute(E& ex, F&& f) {
        detail::execute_in_context(ex, std::forward<F>(f));
    }
} // namespace futures

#endif // FUTURES_EXECUTOR_EXECUTE_HPP
