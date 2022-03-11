//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_FUTURE_LAUNCHER_HPP
#define FUTURES_FUTURES_DETAIL_FUTURE_LAUNCHER_HPP

#include <futures/futures/stop_token.hpp>
#include <futures/futures/traits/future_value.hpp>
#include <futures/futures/detail/continuations_source.hpp>
#include <futures/futures/detail/shared_state.hpp>
#include <futures/futures/detail/traits/is_executor_then_function.hpp>
#include <futures/futures/detail/traits/launch_result.hpp>

namespace futures::detail {
    /// A functor to launch and schedule new futures
    /**
     *  This function is defined as a functor to facilitate friendship in
     *  basic_future
     */
    struct async_future_scheduler
    {
        /// Schedule the function in the executor
        /// This is the internal function async uses to finally schedule the
        /// function after setting the default parameters and converting
        /// policies into scheduling strategies.
        template <
            class FutureOptions,
            class Executor,
            class Function,
            class... Args
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<Executor> &&
                (std::is_invocable_v<Function, Args...> || std::is_invocable_v<Function, stop_token, Args...>)
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        schedule(const Executor& ex, Function&& f, Args&&... args) const {
            // Future traits
            using value_type = launch_result_t<Function, Args...>;
            static constexpr bool is_eager = !FutureOptions::is_always_deferred;
            if constexpr (is_eager) {
                // Create shared state
                auto shared_state = make_initial_state<is_eager, value_type, FutureOptions>(
                    ex,
                    std::forward<Function>(f),
                    std::forward<Args>(args)...);
                basic_future<value_type, FutureOptions> fut(shared_state);

                // Launch task to fulfill the eager promise now
                asio::post(
                    ex,
                    std::move(
                        [state = std::move(shared_state),
                         f = std::forward<Function>(f),
                         args = std::make_tuple(
                             std::forward<Args>(args)...)]() mutable {
                    state->apply_tuple(std::move(f), std::move(args));
                    }));
                return fut;
            } else {
                // Create shared state
                auto op_state
                    = make_initial_state<is_eager, value_type, FutureOptions>(
                    ex,
                    std::forward<Function>(f),
                    std::forward<Args>(args)...);
                basic_future<value_type, FutureOptions> fut(
                    move_if_not_shared_ptr(op_state));
                return fut;
            }
        }

        template <class T>
        static constexpr decltype(auto)
        move_if_not_shared_ptr(T&& v) {
            return std::move<T>(v);
        }

        template <class T>
        static constexpr decltype(auto)
        move_if_not_shared_ptr(std::shared_ptr<T>&& v) {
            return std::forward<T>(v);
        }

        template <
            bool is_eager,
            class ValueType,
            class FutureOptions,
            class Executor,
            class Function,
            class... Args>
        std::enable_if_t<is_eager, shared_state<ValueType, FutureOptions>>
        make_initial_state(const Executor& ex, Function&& f, Args&&... args)
            const {
            using operation_state_t = operation_state<ValueType, FutureOptions>;
            (void) f;
            ((void) args, ...);
            return std::make_shared<operation_state_t>(ex);
        }

        template <
            bool is_eager,
            class ValueType,
            class FutureOptions,
            class Executor,
            class Function,
            class... Args>
        std::enable_if_t<
            !is_eager,
            deferred_operation_state<ValueType, FutureOptions>>
        make_initial_state(const Executor& ex, Function&& f) const {
            return deferred_operation_state<ValueType, FutureOptions>{
                ex,
                std::forward<Function>(f)
            };
        }

        template <
            bool is_eager,
            class ValueType,
            class FutureOptions,
            class Executor,
            class Function,
            class... Args>
        std::enable_if_t<
            !is_eager,
            deferred_operation_state<ValueType, FutureOptions>>
        make_initial_state(const Executor& ex, Function&& f, Args&&... args)
            const {
            return deferred_operation_state<ValueType, FutureOptions>{
                ex,
                std::forward<Function>(f),
                std::forward<Args>(args)...
            };
        }
    };

} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_FUTURE_LAUNCHER_HPP
