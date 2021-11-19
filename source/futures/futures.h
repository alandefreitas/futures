//
// Created by Alan Freitas on 8/1/21.
//

#ifndef FUTURES_FUTURES_H
#define FUTURES_FUTURES_H

/// \file Functions to work with future
///
/// Many of the ideas for these functions are based on:
/// - extensions for concurrency (ISO/IEC TS 19571:2016)
/// - async++
/// - continuable
/// - TBB
///
/// However, we use std::future and the ASIO proposed standard executors (P0443r13, P1348r0, and P1393r0) to allow
/// for better interoperability with the C++ standard.
/// - the async function can accept any standard executor
/// - the async function will use a reasonable default thread pool when no executor is provided
/// - future-concepts allows for new future classes to extend functionality while reusing algorithms
/// - a cancellable future class is provided for more sensitive use cases
/// - the API can be updated as the standard gets updated
/// - the standard algorithms are reimplemented with a preference for parallel operations
///
/// This interoperability comes at a price for continuations, as we might need to poll for when_all/when_any/then
/// events, because std::future does not have internal continuations.
///
/// Although we attempt to replicate these features without recreating the future class with internal continuations,
/// we use a number of heuristics to avoid polling for when_all/when_any/then:
/// - we allow for other future-like classes to be implemented through a future-concept and provide these
///   functionalities at a lower cost whenever we can
/// - `when_all` (or operator&&) returns a when_all_future class, which does not create a new std::future at all
///    and can check directly if futures are ready
/// - `when_any` (or operator||) returns a when_any_future class, which implements a number of heuristics to avoid
///    polling, limit polling time, increased pooling intervals, and only launching the necessary continuation futures
///    for long tasks. (although when_all always takes longer than when_any, when_any involves a number of heuristics
///    that influence its performance)
/// - `then` (or operator>>) returns a new future object that sleeps while the previous future isn't ready
/// - when the standard supports that, this approach based on concepts also serve as extension points to allow
///   for these proxy classes to change their behavior to some other algorithm that makes more sense for futures
///   that support continuations, cancellation, progress, queries, .... More interestingly, the concepts allow
///   for all these possible future types to interoperate.
///
/// \see https://en.cppreference.com/w/cpp/experimental/concurrency
/// \see https://think-async.com/Asio/asio-1.18.2/doc/asio/std_executors.html
/// \see https://github.com/Amanieu/asyncplusplus

#include <future>

// Some executors
#include "detail/default_executor.h"
#include "detail/inline_executor.h"

// Helper functions for std::future
#include "detail/ready_future.h"

// Future types
#include "detail/basic_future.h"

// Future adapters
#include "detail/then.h"
#include "detail/when_all.h"
#include "detail/when_any.h"

namespace futures {
    /** \addtogroup future Futures
     *  @{
     */

    /// \brief Specifies the launch policy for a task executed by the @ref futures::async function
    ///
    /// Most of the time, we want the executor policy. As the @ref async function also accepts an executor,
    /// this option can often be ignored, and is here most for compatibility with the std::async,
    /// so that std::launch can be converted to futures::launch.
    enum class launch {
        new_thread = 1,      // execute on a new thread regarless of executors (like std::async(async))
        deferred = 2,        // execute on the calling thread when result is requested (like std::async(deferred))
        inline_now = 4,      // execute on the calling thread now (uses inline executor)
        executor = 8,        // enqueue task in the default executor (uses default executor)
        executor_now = 16,   // run immediately if inside the default executor (uses default executor)
        executor_later = 32, // enqueue task for later in the default executor (uses default executor)
    };

    /// \brief operator & for launch policies
    constexpr launch operator&(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) & static_cast<int>(y));
    }

    /// \brief operator | for launch policies
    constexpr launch operator|(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) | static_cast<int>(y));
    }

    /// \brief operator ^ for launch policies
    constexpr launch operator^(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) ^ static_cast<int>(y));
    }

    /// \brief operator ~ for launch policies
    constexpr launch operator~(launch x) { return static_cast<launch>(~static_cast<int>(x)); }

    /// \brief operator &= for launch policies
    inline launch &operator&=(launch &x, launch y) { return x = x & y; }

    /// \brief operator |= for launch policies
    inline launch &operator|=(launch &x, launch y) { return x = x | y; }

    /// \brief operator ^= for launch policies
    inline launch &operator^=(launch &x, launch y) { return x = x ^ y; }

    namespace detail {
        struct post_schedule_tag {};
        struct dispatch_schedule_tag {};
        struct defer_schedule_tag {};

        template <typename SchedulePolicy, typename Executor, typename Function,
                  std::enable_if_t<detail::is_executor_then_async_input_v<Executor, Function>, int>>
        async_future_result_of<Function> internal_launch_async(const Executor &ex, Function &&f) {
            // The Function should accept no parameters or the stop token here
            // - No parameters: schedule `cfuture` to allow lazy continuations
            // - Function expects stop token: create stop source and schedule `jcfuture` with access to it
            constexpr bool expects_stop_token = std::is_invocable_v<Function, stop_token>;

            // Create cfuture, and give its continuations source to the callable
            // Or create a jcfuture, and also give a stop token to the callable
            // The value type of our future
            using future_value_type =
                std::conditional_t<expects_stop_token, type_member_or_void_t<std::invoke_result<Function, stop_token>>,
                                   type_member_or_void_t<std::invoke_result<Function>>>;

            // Create stop source (only valid if we expect a continuation)
            stop_source ss = [=]() {
                // this redundancy is an MSVC hack
                constexpr bool st_expects_stop_token = std::is_invocable_v<Function, stop_token>;
                if constexpr (st_expects_stop_token) {
                    return stop_source();
                } else {
                    return stop_source(nostopstate);
                }
            }();

            // Create continuations source
            continuations_source cs;

            // Create promise the input function needs to fulfill
            std::promise<future_value_type> p;
            std::future<future_value_type> std_future = p.get_future();

            // Complete executor task, using result to fulfill the promise, and running continuations
            auto fulfill_promise = [p = std::move(p),              // promise the function needs to fulfill
                                    f = std::forward<Function>(f), // the function that fulfills the promise
                                    continuations = cs,            // continuation source for next futures
                                    token = ss.get_token()         // stop token for stopping the process
            ]() mutable {
                // Arguments we send to the function (with or without token)
                auto func_args = [&]() {
                  constexpr bool fn_expects_stop_token = std::is_invocable_v<Function, stop_token>;
                  if constexpr (!fn_expects_stop_token) {
                        return std::make_tuple();
                    } else {
                        return std::make_tuple(token);
                    }
                }();

                // Run the main function and fulfill its promise
                try {
                    // Check if the function return type isn't void
                    constexpr bool future_returns_void = std::is_same_v<future_value_type, void>;
                    if constexpr (not future_returns_void) {
                        auto state = std::apply(f, std::move(func_args));
                        p.set_value(std::move(state));
                    } else {
                        std::apply(f, std::move(func_args));
                        p.set_value();
                    }
                } catch (...) {
                    p.set_exception(std::current_exception());
                }

                // Run future continuations once the main task is done without errors
                continuations.request_run();
            };

            // Move function to shared location because executors require handles to be copy constructable
            auto run_ptr = std::make_shared<decltype(fulfill_promise)>(std::move(fulfill_promise));
            auto executor_handle = [run_ptr]() { (*run_ptr)(); };

            // Post a handle running the complete function to the executor
            if constexpr (std::is_same_v<SchedulePolicy, detail::post_schedule_tag>) {
                asio::post(ex, executor_handle);
            } else if constexpr (std::is_same_v<SchedulePolicy, detail::dispatch_schedule_tag>) {
                asio::dispatch(ex, executor_handle);
            } else if constexpr (std::is_same_v<SchedulePolicy, detail::defer_schedule_tag>) {
                asio::defer(ex, executor_handle);
            }

            // Set up our future object with access to the extra continuation/stop sources
            async_future_result_of<Function> result_future;
            result_future.set_future(std::make_unique<decltype(std_future)>(std::move(std_future)));
            result_future.set_continuations_source(cs);
            if constexpr (expects_stop_token) {
                result_future.set_stop_source(ss);
            }
            return std::move(result_future);
        }

        template <typename SchedulePolicy, typename Executor, typename Function, typename... Args,
                  std::enable_if_t<detail::is_executor_then_function_v<Executor, Function, Args...>, int> = 0>
        auto internal_wrap_async(const Executor &ex, Function &&f, Args &&...args) {
            return internal_launch_async<SchedulePolicy, Executor>(
                ex, [f = std::forward<Function>(f), args = std::make_tuple(std::forward<Args>(args)...)]() {
                    return std::apply(f, std::move(args));
                });
        }

        template <typename SchedulePolicy, typename Executor, typename Function, typename... Args,
                  std::enable_if_t<detail::is_executor_then_stoppable_function_v<Executor, Function, Args...>, int> = 0>
        auto internal_wrap_async(const Executor &ex, Function &&f, Args &&...args) {
            return internal_launch_async<SchedulePolicy, Executor>(
                ex,
                [f = std::forward<Function>(f), args = std::make_tuple(std::forward<Args>(args)...)](stop_token st) {
                    // Call function with given stop token then the bound original arguments
                    return std::apply(f, std::tuple_cat(std::make_tuple(std::move(st)), std::move(args)));
                });
        }

        template <typename SchedulePolicy, typename Function, typename... Args,
                  std::enable_if_t<detail::is_async_input_non_executor_v<Function, Args...>, int> = 0>
        auto internal_default_async(Function &&f, Args &&...args) {
            return internal_wrap_async<SchedulePolicy>(::futures::make_default_executor(), std::forward<Function>(f),
                                                       std::forward<Args>(args)...);
        }

    } // namespace detail

    /// \brief An extended version of std::async with custom executors instead of policies.
    ///
    /// This version of async will always use an executor to be provided.
    /// If no executor is provided, then the function is run in a default executor made from
    /// the default thread pool.
    ///
    /// \tparam Executor Executor from an execution context
    /// \tparam Function A callable object
    /// \tparam Args Arguments for the Function
    ///
    /// \param ex Executor
    /// \param f Function to execute
    /// \param args Function arguments
    ///
    /// \return A future object with the function results
    template <typename Executor, typename Function, typename... Args,
              std::enable_if_t<detail::is_executor_then_async_input_v<Executor, Function, Args...>, int> = 0>
    decltype(auto) async(const Executor &ex, Function &&f, Args &&...args) {
        return detail::internal_wrap_async<detail::post_schedule_tag>(ex, std::forward<Function>(f),
                                                                      std::forward<Args>(args)...);
    }

    /// \brief Launch an async function with an existing executor according to the policy @ref policy, like std::async
    ///
    /// \tparam Executor Executor from an execution context
    /// \tparam Function A callable object
    /// \tparam Args Arguments for the Function
    ///
    /// \param policy Launch policy
    /// \param ex Executor
    /// \param f Function to execute
    /// \param args Function arguments
    ///
    /// \return A future object with the function results
    template <typename Executor, typename Function, typename... Args,
              std::enable_if_t<detail::is_executor_then_async_input_v<Executor, Function, Args...>, int> = 0>
    decltype(auto) async(launch policy, const Executor &ex, Function &&f, Args &&...args) {
        // Unwrap policies
        const bool new_thread_policy = (policy & launch::new_thread) == launch::new_thread;
        const bool deferred_policy = (policy & launch::deferred) == launch::deferred;
        const bool inline_now_policy = (policy & launch::inline_now) == launch::inline_now;
        const bool executor_policy = (policy & launch::executor) == launch::executor;
        const bool executor_now_policy = (policy & launch::executor_now) == launch::executor_now;
        const bool executor_later_policy = (policy & launch::executor_later) == launch::executor_later;

        // Define executor
        const bool use_default_executor = executor_policy && executor_now_policy && executor_later_policy;
        const bool use_new_thread_executor = not use_default_executor && new_thread_policy;
        const bool use_inline_later_executor = not use_default_executor && deferred_policy;
        const bool use_inline_executor = not use_default_executor && inline_now_policy;
        const bool no_executor_defined =
            !(use_default_executor || use_new_thread_executor || use_inline_later_executor || use_inline_executor);

        if (use_default_executor || no_executor_defined) {
            if (executor_now_policy || inline_now_policy) {
                return detail::internal_wrap_async<detail::dispatch_schedule_tag>(ex, std::forward<Function>(f),
                                                                                  std::forward<Args>(args)...);
            } else if (executor_later_policy || deferred_policy) {
                return detail::internal_wrap_async<detail::defer_schedule_tag>(ex, std::forward<Function>(f),
                                                                               std::forward<Args>(args)...);
            } else {
                return detail::internal_wrap_async<detail::post_schedule_tag>(ex, std::forward<Function>(f),
                                                                              std::forward<Args>(args)...);
            }
        } else {
            if (use_inline_later_executor) {
                return detail::internal_wrap_async<detail::post_schedule_tag>(
                    make_inline_later_executor(), std::forward<Function>(f), std::forward<Args>(args)...);
            } else if (use_inline_executor) {
                return detail::internal_wrap_async<detail::post_schedule_tag>(
                    make_inline_executor(), std::forward<Function>(f), std::forward<Args>(args)...);
            } else {
                return detail::internal_wrap_async<detail::post_schedule_tag>(
                    make_inline_executor(), std::forward<Function>(f), std::forward<Args>(args)...);
            }
        }
    }

    /// \brief Launch an async function according to the policy @ref policy with the default executor
    ///
    /// \tparam Function A callable object
    /// \tparam Args Arguments for the Function
    ///
    /// \param policy Launch policy
    /// \param f Function to execute
    /// \param args Function arguments
    ///
    /// \return A future object with the function results
    template <typename Function, typename... Args,
              std::enable_if_t<detail::is_async_input_non_executor_v<Function, Args...>, int> = 0>
    decltype(auto) async(launch policy, Function &&f, Args &&...args) {
        return async(policy, make_default_executor(), std::forward<Function>(f), std::forward<Args>(args)...);
    }

    /// \brief Launch an async function with the default executor of type @ref default_executor_type
    ///
    /// \tparam Executor Executor from an execution context
    /// \tparam Function A callable object
    /// \tparam Args Arguments for the Function
    ///
    /// \param f Function to execute
    /// \param args Function arguments
    ///
    /// \return A future object with the function results
    template <typename Function, typename... Args,
              std::enable_if_t<detail::is_async_input_non_executor_v<Function, Args...>, int> = 0>
    decltype(auto) async(Function &&f, Args &&...args) {
        return detail::internal_default_async<detail::post_schedule_tag>(std::forward<Function>(f),
                                                                         std::forward<Args>(args)...);
    }

    /// \brief Attempts to schedule a function
    ///
    /// This function attempts to schedule a function, and returns 3 objects:
    /// - The future for the task itself
    /// - A future that indicates if the task got scheduled yet
    /// - A token for canceling the task
    ///
    /// This is mostly useful for recursive tasks, where there might not be room in the executor for
    /// a new task, as depending on recursive tasks for which there is no room is the executor might
    /// block execution.
    ///
    /// Although this is a general solution to allow any executor in the algorithms, executor traits
    /// to identify capacity in executor are much more desirable.
    ///
    template <typename Executor, typename Function, typename... Args,
              std::enable_if_t<detail::is_executor_then_async_input_v<Executor, Function, Args...>, int> = 0>
    decltype(auto) try_async(const Executor &ex, Function &&f, Args &&...args) {
        // Communication flags
        std::promise<void> started_token;
        std::future<void> started = started_token.get_future();
        stop_source cancel_source;

        // Wrap the task in a lambda that sets and checks the flags
        auto do_task = [p = std::move(started_token), cancel_token = cancel_source.get_token(),
                        f](Args &&...args) mutable {
            p.set_value();
            if (cancel_token.stop_requested()) {
                small::throw_exception<std::runtime_error>("task cancelled");
            }
            return std::invoke(f, std::forward<Args>(args)...);
        };

        // Make it copy constructable
        auto do_task_ptr = std::make_shared<decltype(do_task)>(std::move(do_task));
        auto do_task_handle = [do_task_ptr](Args &&...args) { return (*do_task_ptr)(std::forward<Args>(args)...); };

        // Launch async
        using internal_result_type = std::decay_t<decltype(std::invoke(f, std::forward<Args>(args)...))>;
        cfuture<internal_result_type> rhs = async(ex, do_task_handle, std::forward<Args>(args)...);

        // Return future and tokens
        return std::make_tuple(std::move(rhs), std::move(started), cancel_source);
    }

    /// \brief Very simple version syntax sugar for types that pass the Future concept: future.wait() / future.get()
    /// This syntax is most useful for cases where we are immediately requesting the future result
    template <typename Future, std::enable_if_t<is_future_v<Future>, int> = 0> decltype(auto) await(Future &&f) {
        return f.get();
    }

    /** @} */ // \addtogroup future Futures
} // namespace futures
#endif // FUTURES_FUTURES_H
