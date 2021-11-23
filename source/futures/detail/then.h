//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_THEN_H
#define FUTURES_THEN_H

#include <future>

#include "traits/is_callable.h"
#include "traits/is_executor_then_continuation.h"
#include "traits/is_future.h"
#include "traits/unwrap_future.h"

#include "basic_future.h"
#include "tuple_algorithm.h"
#include <version>

namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    namespace detail {
        /// \brief Create a decay copy
        /// As in the standard extensions for concurrency
        template <class T> std::decay_t<T> decay_copy(T &&v) { return std::forward<T>(v); }

        /// \brief Unwrap the results from `before` future object and give them to `continuation`
        /// \return Continuation result
        template <class Future, typename Function, typename... Args>
        continuation_result_t<Function, Future, std::tuple<Args...>>
        unwrap_and_continue(Future &&before_future, Function &&continuation, Args &&...args) {
            // Common continuations for basic_future
            if constexpr (detail::is_void_continuation_v<Function, Future, Args...>) {
                before_future.get();
                return continuation(std::forward<Args>(args)...);
            } else if constexpr (detail::is_direct_continuation_v<Function, Future, Args...>) {
                return continuation(std::forward<Args>(args)..., before_future.get());
            } else if constexpr (detail::is_lvalue_continuation_v<Function, Future, Args...>) {
                auto ref = before_future.get();
                return continuation(std::forward<Args>(args)..., ref);
            } else if constexpr (detail::is_rvalue_continuation_v<Function, Future, Args...>) {
                auto ref = before_future.get();
                return continuation(std::forward<Args>(args)..., std::move(ref));
            } else if constexpr (detail::is_double_unwrap_continuation_v<Function, Future, Args...>) {
                return continuation(std::forward<Args>(args)..., before_future.get().get());
            }
            // Common continuations for when_all
            else if constexpr (detail::is_tuple_explode_continuation_v<Function, Future, Args...>) {
                return std::apply(continuation,
                                  std::tuple_cat(std::make_tuple(std::forward<Args>(args)...), before_future.get()));
            } else if constexpr (detail::is_tuple_unwrap_continuation_v<Function, Future, Args...>) {
                return transform_and_apply(
                    continuation,
                    [](auto &&el) {
                        if constexpr (not is_future_v<std::decay_t<decltype(el)>>) {
                            return el;
                        } else {
                            return el.get();
                        }
                    },
                    std::tuple_cat(std::make_tuple(std::forward<Args>(args)...), before_future.get()));
            } else if constexpr (detail::is_vector_unwrap_continuation_v<Function, Future, Args...>) {
                using future_vector_type = unwrap_future_t<Future>;
                future_vector_type futures = before_future.get();
                using future_vector_value_type = typename future_vector_type::value_type;
                using unwrap_future_vector_value_type = unwrap_future_t<future_vector_value_type>;
                using unwrap_future_vector_type = ::futures::small_vector<unwrap_future_vector_value_type>;
                unwrap_future_vector_type values;
                std::transform(futures.begin(), futures.end(), std::back_inserter(values),
                               [](future_vector_value_type &f) { return f.get(); });
                return continuation(std::forward<Args>(args)..., values);
            }
            // Common continuations for when_any futures
            else if constexpr (detail::is_when_any_split_continuation_v<Function, Future, Args...>) {
                auto w = before_future.get();
                return std::apply(continuation,
                                  std::make_tuple(std::forward<Args>(args)..., w.index, std::move(w.tasks)));
            } else if constexpr (detail::is_when_any_explode_continuation_v<Function, Future, Args...>) {
                auto w = before_future.get();
                return std::apply(continuation, std::tuple_cat(std::make_tuple(std::forward<Args>(args)..., w.index),
                                                               std::move(w.tasks)));
            } else if constexpr (detail::is_when_any_element_continuation_v<Function, Future, Args...>) {
                auto w = before_future.get();
                auto nth_future = [&]() {
                    if constexpr (is_tuple_v<std::decay_t<decltype(w.tasks)>>) {
                        return std::move(get(std::move(w.tasks), w.index));
                    } else {
                        return std::move(w.tasks[w.index]);
                    }
                }();
                return continuation(std::forward<Args>(args)..., std::move(nth_future));
            } else if constexpr (detail::is_when_any_unwrap_continuation_v<Function, Future, Args...>) {
                auto w = before_future.get();
                auto nth_value = [&]() {
                    if constexpr (is_tuple_v<std::decay_t<decltype(w.tasks)>>) {
                        return std::move(get(std::move(w.tasks), w.index, [](auto &el) { return el.get(); }));
                    } else {
                        return std::move(w.tasks[w.index]).get();
                    }
                }();
                return continuation(std::forward<Args>(args)..., std::move(nth_value));
            } else {
                small::throw_exception<std::logic_error>(
                    "Continuation unwrapping detected as possible but not handled");
                std::abort();
            }
        }

        template <typename Executor, typename Function, class Future,
                  std::enable_if_t<is_executor_then_continuation_v<Executor, Function, Future>, int>>
        then_result_of_t<Function, Future> internal_then(const Executor &ex, Future &&before, Function &&after) {
            // Deduce types
            using result_future_type = detail::then_result_of_t<Function, Future>;
            using result_value_type = unwrap_future_t<result_future_type>;

            // Set up continuation stop source
            stop_source ss = [&] {
              constexpr bool result_has_stop_token = has_stop_token_v<result_future_type>;
              if constexpr (result_has_stop_token) {
                  constexpr bool before_has_stop_token = has_stop_token_v<Future>;
                  constexpr bool before_is_shared = is_shared_future_v<Future>;
                  constexpr bool propagate_stop_token = before_has_stop_token && not before_is_shared;
                  constexpr bool function_expects_stop_token = is_future_continuation_v<Function, Future, stop_token>;
                  constexpr bool result_shares_stop_source = propagate_stop_token && not function_expects_stop_token;
                  if constexpr (result_shares_stop_source) {
                        return before.get_stop_source();
                    } else {
                        return stop_source();
                    }
                } else {
                    return stop_source(nostopstate);
                }
            }();

            // Set up a new continuations source
            detail::continuations_source after_cs;

            // Store previous continuation source, so we can access it after it maybe gets moved into the result lambda
            continuations_source before_cs = [&]() {
              constexpr bool before_is_lazy_continuable = is_lazy_continuable_v<Future>;
              if constexpr (before_is_lazy_continuable) {
                    return before.get_continuations_source();
                } else {
                    return continuations_source(nocontinuationsstate);
                }
            }();

            // Create promise the continuation function needs to fulfill
            std::promise<result_value_type> p;
            std::future<result_value_type> std_future = p.get_future();

            // Set the complete executor task, using result to fulfill the promise, and running continuations
            auto fulfill_promise = [p = std::move(p),                                    // after result promise
                                    continuation = std::move(detail::decay_copy(after)), // the continuation func
                                    before_future =
                                        std::move(move_or_share(std::forward<Future>(before))), // the previous future
                                    st = ss.get_token(), // maybe shared stop token
                                    after_cs             // continuation source for after
            ]() mutable {
                constexpr static bool continuation_expects_stop_token =
                    is_future_continuation_v<std::decay_t<Function>, Future, stop_token>;

                // Arguments we send to the unwrap_and_continue function (with or without token)
                auto apply_args = [&]() {
                    if constexpr (not continuation_expects_stop_token) {
                        return std::make_tuple(std::move(before_future), std::move(continuation));
                    } else {
                        return std::make_tuple(std::move(before_future), std::move(continuation), st);
                    }
                }();

                // Unwrap and continue functor (for std::apply)
                auto unwrap_and_continue = [](auto &&...unwrap_args) -> decltype(auto) {
                    return detail::unwrap_and_continue(std::forward<decltype(unwrap_args)>(unwrap_args)...);
                };

                // Unwrap and run the main function to fulfill its promise
                try {
                    constexpr bool future_returns_void = std::is_same_v<result_value_type, void>;
                    if constexpr (not future_returns_void) {
                        auto state = std::apply(unwrap_and_continue, std::move(apply_args));
                        p.set_value(std::move(state));
                    } else {
                        std::apply(unwrap_and_continue, std::move(apply_args));
                        p.set_value();
                    };
                } catch (...) {
                    p.set_exception(std::current_exception());
                }

                // Run future continuations once the main task is done
                after_cs.request_run();
            };

            // Move function to shared location because executors require handles to be copy constructable
            auto continue_ptr = std::make_shared<decltype(fulfill_promise)>(std::move(fulfill_promise));
            auto executor_handle = [continue_ptr]() { (*continue_ptr)(); };

            constexpr bool before_is_lazy_continuable = is_lazy_continuable_v<Future>;
            if constexpr (before_is_lazy_continuable) {
                // Attach continuation to previous future.
                // - Continuation is posted when previous is done
                // - Continuation is posted immediately if previous is already done
                before_cs.emplace_continuation(ex, [ex, executor_handle]() { asio::post(ex, executor_handle); });
            } else {
                // We defer the task in the executor because the input doesn't have lazy continuations.
                // The executor will take care of this running later.
                asio::defer(ex, executor_handle);
            }

            // Set up our future object with access to the extra continuation/stop sources
            result_future_type result;
            result.set_future(std::make_unique<std::future<result_value_type>>(std::move(std_future)));
            result.set_continuations_source(after_cs);
            constexpr bool result_has_stop_token = has_stop_token_v<result_future_type>;
            if constexpr (result_has_stop_token) {
                result.set_stop_source(ss);
            }
            return std::move(result);
        }

    } // namespace detail

    /// \brief Schedule a continuation function to a future
    ///
    /// This creates a continuation that gets executed when the before future is over.
    /// The continuation needs to be invocable with the return type of the previous future.
    ///
    /// This function works for all kinds of futures but behavior depends on the input:
    /// - If previous future is continuable, attach the function to the continuation list
    /// - If previous future is not continuable (such as std::future), post to execution with deferred policy
    /// In both cases, the result becomes a cfuture or jcfuture.
    ///
    /// Stop tokens are also propagated:
    /// - If after function expects a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with shared stop source
    ///   - Otherwise:                                      return jcfuture with new stop source
    /// - If after function does not expect a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with shared stop source
    ///   - Otherwise:                                      return cfuture with no stop source
    ///
    /// \return A continuation to the before future
    template <typename Executor, typename Function, class Future,
              std::enable_if_t<detail::is_executor_then_continuation_v<Executor, Function, Future>, int> = 0>
    detail::then_result_of_t<Function, Future> then(const Executor &ex, Future &&before, Function &&after) {
        return detail::internal_then(ex, std::forward<Future>(before), std::forward<Function>(after));
    }

    /// \brief Schedule a continuation function to a future, allow an executor as second parameter
    ///
    /// \see @ref then
    template <class Future, typename Executor, typename Function,
              std::enable_if_t<detail::is_executor_then_continuation_v<Executor, Function, Future>, int> = 0>
    auto then(Future &&before, const Executor &ex, Function &&after) {
        return then(ex, std::forward<Future>(before), std::forward<Function>(after));
    }

    /// \brief Schedule a continuation function to a future with the default executor
    ///
    /// \return A continuation to the before future
    ///
    /// \see @ref then
    template <class Future, typename Function,
              std::enable_if_t<detail::is_continuation_non_executor_v<Function, Future>, int> = 0>
    auto then(Future &&before, Function &&after) {
        return then(::futures::make_default_executor(), std::forward<Future>(before), std::forward<Function>(after));
    }

    /// \brief Operator to schedule a continuation function to a future
    ///
    /// \return A continuation to the before future
    template <class Future, typename Function,
              std::enable_if_t<detail::is_continuation_non_executor_v<Function, Future>, int> = 0>
    auto operator>>(Future &&before, Function &&after) {
        return then(std::forward<Future>(before), std::forward<Function>(after));
    }

    /// \brief Schedule a continuation function to a future with a custom executor
    ///
    /// \return A continuation to the before future
    template <class Executor, class Future, typename Function,
              std::enable_if_t<detail::is_executor_then_continuation_v<Executor, Function, Future>, int> = 0>
    auto operator>>(Future &&before, std::pair<const Executor &, Function &> &&after) {
        return then(after.first, std::forward<Future>(before), std::forward<Function>(after.second));
    }

    /// \brief Create a proxy pair to schedule a continuation function to a future with a custom executor
    ///
    /// For this operation, we needed an operator with higher precedence than operator>>
    /// Our options are: +, -, *, /, %, &, !, ~.
    /// Although + seems like an obvious choice, % is the one that leads to less conflict with other functions.
    ///
    /// \return A proxy pair to schedule execution
    template <class Executor, typename Function, typename... Args,
              std::enable_if_t<asio::is_executor<Executor>::value && not asio::is_executor<Function>::value &&
                                   not is_callable_v<std::decay_t<Executor>> && is_callable_v<std::decay_t<Function>>,
                               int> = 0>
    auto operator%(const Executor &ex, Function &&after) {
        return std::make_pair(std::cref(ex), std::ref(after));
    }

    /** @} */  // \addtogroup adaptors Adaptors
} // namespace futures

#endif // FUTURES_THEN_H
