//
// Created by Alan Freitas on 8/17/21.
//

#ifndef CPP_MANIFEST_DEFAULT_EXECUTOR_H
#define CPP_MANIFEST_DEFAULT_EXECUTOR_H

#ifdef _WIN32
#include <SDKDDKVer.h>
#endif

#include <asio.hpp>

namespace futures {
    /// \brief The default execution context for async operations, unless otherwise stated
    ///
    /// Unless an executor is explicitly provided, this is the executor we use
    /// for async operations.
    ///
    /// This is the ASIO thread pool execution context with a default number of threads.
    /// However, the default execution context (and its type) might change in other
    /// versions of this library if something more general comes along. As the standard
    /// for executors gets adopted, libraries are likely to provide better implementations.
    ///
    /// Also note that executors might not allow work-stealing. This needs to be taken into
    /// account when implementing algorithms with recursive tasks. One common options is to
    /// use `try_async` for recursive tasks.
    ///
    /// Also note that, in the executors notation, the pool is an execution context but not
    /// an executor:
    /// - Execution context: a place where we can execute functions
    /// - A thread pool is an execution context, not an executor
    ///
    /// An execution context is:
    /// - Usually long lived
    /// - Non-copyable
    /// - May contain additional state, such as timers, and threads
    ///
    using default_execution_context_type = asio::thread_pool;

    /// \brief Create an instance of the default execution context
    default_execution_context_type &default_execution_context();

    /// \brief Create an Asio thread pool executor for the default thread pool
    /// In the executors notation:
    /// - Executor: set of rules governing where, when and how to run a function object
    ///   - A thread pool is an execution context for which we can create executors pointing to the pool.
    ///   - The executor rule for the default thread pool executor is to run function objects in the pool
    ///     and nowhere else.
    ///
    /// An executor is:
    /// - Lightweight and copyable (just references and pointers to the execution context).
    /// - May be long or short lived.
    /// - May be customized on a fine-grained basis, such as exception behavior, and order
    ///
    /// There might be many executor types associated with with the same execution context.
    ///
    default_execution_context_type::executor_type make_default_executor();

    /// \brief Default executor type as a constant trait for future_base functions
    using default_executor_type = default_execution_context_type::executor_type;

    /// \brief Improved version of std::hardware_concurrency:
    /// - It never returns 0, 1 is returned instead.
    /// - It is guaranteed to remain constant for the duration of the program.
    std::size_t hardware_concurrency() noexcept;

    /// \brief Determine a reasonable minimum grain size depending on the number of elements in a sequence
    inline std::size_t make_grain_size(std::size_t n) {
        return std::clamp(n / (8 * hardware_concurrency()), size_t(1), size_t(2048));
    }

    /// \brief Determine if type is an executor
    template <typename T> using is_executor = asio::is_executor<T>;

    /// \brief Determine if type is an executor
    template <typename T> constexpr bool is_executor_v = is_executor<T>::value;

} // namespace futures

#endif // CPP_MANIFEST_DEFAULT_EXECUTOR_H
