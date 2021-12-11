//
// Copyright (c) alandefreitas 11/30/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_ASYNC_H
#define FUTURES_ASYNC_H

#include <futures/executor/inline_executor.h>

#include <futures/futures/detail/empty_base.h>
#include <futures/futures/detail/traits/async_result_of.h>

#include <futures/futures/await.h>
#include <futures/futures/launch.h>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup launch Launch
     *  @{
     */
    /** \addtogroup launch-algorithms Launch Algorithms
     *  @{
     */

    namespace detail {
        enum class schedule_future_policy {
            /// \brief Internal tag indicating the executor should post the execution
            post,
            /// \brief Internal tag indicating the executor should dispatch the execution
            dispatch,
            /// \brief Internal tag indicating the executor should defer the execution
            defer,
        };

        /// \brief A single trait to validate and constraint futures::async input types
        template <class Executor, class Function, typename... Args>
        using is_valid_async_input = std::disjunction<is_executor_then_function<Executor, Function, Args...>,
                                                      is_executor_then_stoppable_function<Executor, Function, Args...>>;

        /// \brief A single trait to validate and constraint futures::async input types
        template <class Executor, class Function, typename... Args>
        constexpr bool is_valid_async_input_v = is_valid_async_input<Executor, Function, Args...>::value;

        /// \brief Create a new stop source for the new shared state
        template <bool expects_stop_token> auto create_stop_source() {
            if constexpr (expects_stop_token) {
                stop_source ss;
                return std::make_pair(ss, ss.get_token());
            } else {
                return std::make_pair(empty_value, empty_value);
            }
        }

        /// This function is defined as a functor to facilitate friendship in basic_future
        struct async_future_scheduler {
            /// This is a functor to fulfill a promise in a packaged task
            /// Handle to fulfill promise. Asio requires us to create a handle because
            /// callables need to be *copy constructable*. Continuations also require us to create an
            /// extra handle because we need to run them after the function is over.
            template <class StopToken, class Function, class Task, class... Args>
            class promise_fulfill_handle
                : public maybe_empty<StopToken>, // stop token for stopping the process is represented in base class
                  public maybe_empty<std::tuple<Args...>> // arguments bound to the function to fulfill the promise also
                                                          // have empty base opt
            {
              public:
                promise_fulfill_handle(Task &&pt, std::tuple<Args...> &&args, continuations_source cs,
                                       const StopToken &st)
                    : maybe_empty<StopToken>(st), maybe_empty<std::tuple<Args...>>(std::move(args)),
                      pt_(std::forward<Task>(pt)), continuations_(std::move(cs)) {}

                void operator()() {
                    // Fulfill promise
                    if constexpr (std::is_invocable_v<Function, StopToken, Args...>) {
                        std::apply(pt_, std::tuple_cat(std::make_tuple(token()), std::move(args())));
                    } else {
                        std::apply(pt_, std::move(args()));
                    }
                    // Run future continuations
                    continuations_.request_run();
                }

                /// \brief Get stop token from the base class as function for convenience
                const StopToken &token() const { return maybe_empty<StopToken>::get(); }

                /// \brief Get stop token from the base class as function for convenience
                StopToken &token() { return maybe_empty<StopToken>::get(); }

                /// \brief Get args from the base class as function for convenience
                const std::tuple<Args...> &args() const { return maybe_empty<std::tuple<Args...>>::get(); }

                /// \brief Get args from the base class as function for convenience
                std::tuple<Args...> &args() { return maybe_empty<std::tuple<Args...>>::get(); }

              private:
                /// \brief Task we need to fulfill the promise and its shared state
                Task pt_;

                /// \brief Continuation source for next futures
                continuations_source continuations_;
            };

            /// \brief Schedule the function in the executor
            /// This is the internal function async uses to finally schedule the function after setting the
            /// default parameters and converting policies into scheduling strategies.
            template <typename Executor, typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
                      ,
                      std::enable_if_t<is_valid_async_input_v<Executor, Function, Args...>, int> = 0
#endif
                      >
            async_result_of_t<Function, Args...> operator()(schedule_future_policy policy, const Executor &ex,
                                                            Function &&f, Args &&...args) const {
                using future_value_type = async_result_value_type_t<Function, Args...>;
                using future_type = async_result_of_t<Function, Args...>;

                // Shared sources
                constexpr bool expects_stop_token = std::is_invocable_v<Function, stop_token, Args...>;
                auto [s_source, s_token] = create_stop_source<expects_stop_token>();
                continuations_source cs;

                // Set up shared state
                using packaged_task_type =
                    std::conditional_t<expects_stop_token,
                                       packaged_task<future_value_type(stop_token, std::decay_t<Args>...)>,
                                       packaged_task<future_value_type(std::decay_t<Args>...)>>;
                packaged_task_type pt{std::forward<Function>(f)};
                future_type result{pt.template get_future<future_type>()};
                result.set_continuations_source(cs);
                if constexpr (expects_stop_token) {
                    result.set_stop_source(s_source);
                }
                promise_fulfill_handle<std::decay_t<decltype(s_token)>, Function, packaged_task_type, Args...>
                    fulfill_promise(std::move(pt), std::make_tuple(std::forward<Args>(args)...), cs, s_token);

                // Fire-and-forget: Post a handle running the complete function to the executor
                switch (policy) {
                case schedule_future_policy::dispatch:
                    asio::dispatch(ex, std::move(fulfill_promise));
                    break;
                case schedule_future_policy::defer:
                    asio::defer(ex, std::move(fulfill_promise));
                    break;
                default:
                    asio::post(ex, std::move(fulfill_promise));
                    break;
                }
                return result;
            }
        };
        constexpr async_future_scheduler schedule_future;
    } // namespace detail

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
    template <typename Executor, typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_valid_async_input_v<Executor, Function, Args...>, int> = 0
#endif
              >
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
        const bool use_new_thread_executor = (!use_default_executor) && new_thread_policy;
        const bool use_inline_later_executor = (!use_default_executor) && deferred_policy;
        const bool use_inline_executor = (!use_default_executor) && inline_now_policy;
        const bool no_executor_defined =
            !(use_default_executor || use_new_thread_executor || use_inline_later_executor || use_inline_executor);

        // Define schedule policy
        detail::schedule_future_policy schedule_policy;
        if (use_default_executor || no_executor_defined) {
            if (executor_now_policy || inline_now_policy) {
                schedule_policy = detail::schedule_future_policy::dispatch;
            } else if (executor_later_policy || deferred_policy) {
                schedule_policy = detail::schedule_future_policy::defer;
            } else {
                schedule_policy = detail::schedule_future_policy::post;
            }
        } else {
            schedule_policy = detail::schedule_future_policy::post;
        }

        return detail::schedule_future(schedule_policy, ex, std::forward<Function>(f), std::forward<Args>(args)...);
    }

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
    template <typename Executor, typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_valid_async_input_v<Executor, Function, Args...>, int> = 0
#endif
              >
    detail::async_result_of_t<Function, Args...> async(const Executor &ex, Function &&f, Args &&...args) {
        return async(launch::async, ex, std::forward<Function>(f), std::forward<Args>(args)...);
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
    template <typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_async_input_non_executor_v<Function, Args...>, int> = 0
#endif
              >
    detail::async_result_of_t<Function, Args...> async(launch policy, Function &&f, Args &&...args) {
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
    template <typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_async_input_non_executor_v<Function, Args...>, int> = 0
#endif
              >
    detail::async_result_of_t<Function, Args...> async(Function &&f, Args &&...args) {
        return async(launch::async, ::futures::make_default_executor(), std::forward<Function>(f),
                     std::forward<Args>(args)...);
    }

    /** @} */
    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ASYNC_H
