//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_FUTURE_LAUNCHER_HPP
#define FUTURES_DETAIL_FUTURE_LAUNCHER_HPP

#include <futures/stop_token.hpp>
#include <futures/executor/execute.hpp>
#include <futures/traits/future_value.hpp>
#include <futures/detail/launch.hpp>
#include <futures/detail/shared_state.hpp>
#include <futures/detail/traits/launch_result.hpp>
#include <futures/detail/utility/move_only_function.hpp>
#include <futures/detail/deps/boost/core/ignore_unused.hpp>
#include <type_traits>

namespace futures {
    namespace detail {
        // A functor to launch and schedule new futures
        /*
         *  This function is defined as a functor to facilitate friendship in
         *  basic_future
         */
        struct async_future_scheduler {
            /// Schedule the function in the executor
            /// This is the internal function async uses to finally schedule the
            /// function after setting the default parameters and converting
            /// policies into scheduling strategies.
            FUTURES_TEMPLATE(
                class FutureOptions,
                class Executor,
                class Function,
                class... Args)
            (requires is_executor_v<Executor>
             && (is_invocable_v<Function, Args...>
                 || is_invocable_v<Function, stop_token, Args...>) )
                FUTURES_DETAIL(decltype(auto))
                    schedule(Executor const& ex, Function&& f, Args&&... args)
                        const {
                // Future traits
                static constexpr bool is_eager
                    = !FutureOptions::is_always_deferred;
                return launch_impl<FutureOptions>(
                    mp_bool<is_eager>{},
                    ex,
                    std::forward<Function>(f),
                    std::forward<Args>(args)...);
            }

            template <
                class FutureOptions,
                class Executor,
                class Function,
                class... Args>
            FUTURES_DETAIL(decltype(auto))
            launch_impl(
                std::true_type /* is_eager */,
                Executor const& ex,
                Function&& f,
                Args&&... args) const {
                // Future traits
                using value_type = launch_result_t<Function, Args...>;

                // Create shared state
                auto shared_state
                    = make_initial_state<true, value_type, FutureOptions>(
                        ex,
                        std::forward<Function>(f),
                        std::forward<Args>(args)...);
                basic_future<value_type, FutureOptions> fut(shared_state);

                // Launch task to fulfill the eager promise now
                execute(
                    ex,
                    std::move(
                        [state = std::move(shared_state),
                         f = std::forward<Function>(f),
                         args = std::make_tuple(
                             std::forward<Args>(args)...)]() mutable {
                    state->apply_tuple(std::move(f), std::move(args));
                    }));
                return fut;
            }

            template <
                class FutureOptions,
                class Executor,
                class Function,
                class... Args>
            FUTURES_DETAIL(decltype(auto))
            launch_impl(
                std::false_type /* is_eager */,
                Executor const& ex,
                Function&& f,
                Args&&... args) const {
                // Future traits
                using value_type = launch_result_t<Function, Args...>;

                // Create shared state
                auto op_state
                    = make_initial_state<false, value_type, FutureOptions>(
                        ex,
                        std::forward<Function>(f),
                        std::forward<Args>(args)...);
                basic_future<value_type, FutureOptions> fut(
                    move_if_not_shared_ptr(op_state));
                return fut;
            }

            template <class T>
            static constexpr FUTURES_DETAIL(decltype(auto))
            move_if_not_shared_ptr(T&& v) {
                return std::move<T>(v);
            }

            template <class T>
            static constexpr FUTURES_DETAIL(decltype(auto))
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
            make_initial_state(Executor const& ex, Function&& f, Args&&... args)
                const {
                using operation_state_t
                    = operation_state<ValueType, FutureOptions>;
                // An eager operation state doesn't store the function and its
                // arguments
                boost::ignore_unused(f, args...);
                return std::allocate_shared<operation_state_t>(
                    default_futures_allocator<operation_state_t>{},
                    ex);
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
            make_initial_state(Executor const& ex, Function&& f) const {
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
            make_initial_state(Executor const& ex, Function&& f, Args&&... args)
                const {
                return deferred_operation_state<ValueType, FutureOptions>{
                    ex,
                    std::forward<Function>(f),
                    std::forward<Args>(args)...
                };
            }
        };

    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_FUTURE_LAUNCHER_HPP
