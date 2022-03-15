//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_LAUNCH_HPP
#define FUTURES_FUTURES_LAUNCH_HPP

#include <futures/executor/inline_executor.hpp>
#include <futures/futures/await.hpp>
#include <futures/futures/basic_future.hpp>
#include <futures/detail/utility/maybe_empty.hpp>
#include <futures/futures/detail/future_launcher.hpp>
#include <futures/futures/detail/traits/is_future_options.hpp>
#include <futures/futures/detail/traits/launch_result.hpp>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup launch Launch
     *  @{
     */
    /** @addtogroup launch-algorithms Launch Algorithms
     *
     * \brief Function to launch and schedule future tasks
     *
     * This module contains functions we can use to launch and schedule tasks.
     * Tasks can be scheduled lazily instead of eagerly to avoid a race between
     * the task and its dependencies.
     *
     * When tasks are scheduled eagerly, the function @ref async provides an
     * alternatives to launch tasks on specific executors instead of creating a
     * new thread for each asynchronous task.
     *
     *  @{
     */

    namespace detail {
        /// Determine the options for a basic_future returned from async
        template <class Executor, class Function, class... Args>
        struct async_future_options
        {
            using type = conditional_append_future_option_t<
                std::is_invocable_v<std::decay_t<Function>, stop_token, Args...>,
                stoppable_opt,
                future_options<executor_opt<Executor>, continuable_opt>>;
            static_assert(is_future_options_v<type>);
        };

        template <class Executor, class Function, class... Args>
        using async_future_options_t =
            typename async_future_options<Executor, Function, Args...>::type;

        /// Determine the options for a basic_future returned from schedule
        template <class Executor, class Function, class... Args>
        struct schedule_future_options
        {
        private:
            // base options
            using base_options = future_options<executor_opt<Executor>>;
            static_assert(is_future_options_v<base_options>);

            // maybe include stop token
            using maybe_stoppable_options = conditional_append_future_option_t<
                std::is_invocable_v<std::decay_t<Function>, stop_token, Args...>,
                stoppable_opt,
                future_options<executor_opt<Executor>>>;
            static_assert(is_future_options_v<maybe_stoppable_options>);

            // include deferred option
            using deferred_options = append_future_option_t<
                always_deferred_opt,
                maybe_stoppable_options>;
            static_assert(is_future_options_v<deferred_options>);

            // include deferred function type
            using typed_deferred_function = std::conditional_t<
                sizeof...(Args) == 0,
                Function,
                bind_deferred_state_args<Function, Args...>>;
            static_assert(std::is_invocable_v<typed_deferred_function>);
            using typed_deferred_options = append_future_option_t<
                deferred_function_opt<typed_deferred_function>,
                deferred_options>;
            static_assert(is_future_options_v<typed_deferred_options>);
        public:
            using type = typed_deferred_options;
            static_assert(is_future_options_v<type>);
        };

        template <class Executor, class Function, class... Args>
        using schedule_future_options_t =
            typename schedule_future_options<Executor, Function, Args...>::type;
    } // namespace detail

    /// Launch an asynchronous task with the specified executor
    /**
     *  This version of the async function will always use the specified
     *  executor instead of creating a new thread.
     *
     *  If no executor is provided, then the function is run in a default
     *  executor created from the default thread pool. The default executor
     *  also ensures the function will not launch one thread per task.
     *
     *  The task might accept a stop token as its first parameter, in which
     *  case the function returns a continuable and stoppable future type.
     *  Otherwise, this function returns a continuable future type.
     *
     *  @par Example
     *  @code
     *  auto f = async(ex, []() { return 2; });
     *  std::cout << f.get() << std::endl; // 2
     *  @endcode
     *
     *  @see
     *       \ref basic_future
     *
     *  @tparam Executor Executor from an execution context
     *  @tparam Function A callable object
     *  @tparam Args Arguments for the Function
     *
     *  @param ex Executor
     *  @param f Function to execute
     *  @param args Function arguments
     *
     *  @return An eager future object whose shared state refers to the task
     *  result. The type of this future object depends on the task. If the task
     *  expects a @ref stop_token, the future will return a continuable,
     * stoppable, eager future. Otherwise, the function will return a
     * continuable eager future.
     */
    template <
        class Executor,
        class Function,
        class... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            is_executor_v<Executor> &&
            (std::is_invocable_v<Function, Args...> ||
             std::is_invocable_v<Function, stop_token, Args...>),
            // clang-format on
            int> = 0
#endif
        >
    decltype(auto)
    async(const Executor &ex, Function &&f, Args &&...args) {
        return detail::async_future_scheduler{}
            .schedule<
                detail::async_future_options_t<Executor, Function, Args...>>(
                ex,
                std::forward<Function>(f),
                std::forward<Args>(args)...);
    }

    /// Launch an asynchronous task with the default executor
    /**
     * @tparam Executor Executor from an execution context
     * @tparam Function A callable object
     * @tparam Args Arguments for the Function
     *
     * @param f Function to execute
     * @param args Function arguments
     *
     * @return A future object with the function results
     */
    template <
        class Function,
        class... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            !is_executor_v<Function> &&
            (std::is_invocable_v<Function, Args...> ||
             std::is_invocable_v<Function, stop_token, Args...>),
            // clang-format on
            int> = 0
#endif
        >
    decltype(auto)
    async(Function &&f, Args &&...args) {
        return detail::async_future_scheduler{}
            .schedule<detail::async_future_options_t<
                default_executor_type,
                Function,
                Args...>>(
                ::futures::make_default_executor(),
                std::forward<Function>(f),
                std::forward<Args>(args)...);
    }

    /// Schedule an asynchronous task with the specified executor
    /**
     *  This function schedules a deferred future. The task will only
     *  be launched in the executor when some other execution context waits
     *  for the value associated to this future.
     *
     *  @see
     *       \ref basic_future
     *
     *  @tparam Executor Executor from an execution context
     *  @tparam Function A callable object
     *  @tparam Args Arguments for the Function
     *
     *  @param ex Executor
     *  @param f Function to execute
     *  @param args Function arguments
     *
     *  @return A deferred future object whose shared state refers to the task
     *  result. The type of this future object depends on the task. If the task
     *  expects a @ref stop_token, the future will return a stoppable deferred
     *  future. Otherwise, the function will return a deferred future.
     */
    template <
        class Executor,
        class Function,
        class... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            is_executor_v<Executor> &&
            (std::is_invocable_v<Function, Args...> ||
             std::is_invocable_v<Function, stop_token, Args...>),
            // clang-format on
            int> = 0
#endif
        >
    decltype(auto)
    schedule(const Executor &ex, Function &&f, Args &&...args) {
        return detail::async_future_scheduler{}
            .schedule<
                detail::schedule_future_options_t<Executor, Function, Args...>>(
                ex,
                std::forward<Function>(f),
                std::forward<Args>(args)...);
    }

    /// Schedule an asynchronous task with the default executor
    /**
     *  @tparam Executor Executor from an execution context
     *  @tparam Function A callable object
     *  @tparam Args Arguments for the Function
     *
     *  @param f Function to execute
     *  @param args Function arguments
     *
     *  @return A future object with the function results
     */
    template <
        class Function,
        class... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            !is_executor_v<Function> &&
            (std::is_invocable_v<Function, Args...> ||
             std::is_invocable_v<Function, stop_token, Args...>),
            // clang-format on
            int> = 0
#endif
        >
    decltype(auto)
    schedule(Function &&f, Args &&...args) {
        return detail::async_future_scheduler{}
            .schedule<detail::schedule_future_options_t<
                default_executor_type,
                Function,
                Args...>>(
                ::futures::make_default_executor(),
                std::forward<Function>(f),
                std::forward<Args>(args)...);
    }

    /** @} */
    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURES_LAUNCH_HPP
