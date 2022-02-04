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
#include <futures/futures/detail/shared_task.hpp>
#include <futures/futures/detail/traits/is_executor_then_function.hpp>
#include <futures/futures/detail/traits/launch_result.hpp>

namespace futures::detail {
    /// This function is defined as a functor to facilitate friendship in
    /// basic_future
    template <bool is_eager>
    struct async_future_scheduler
    {
        template <
            class value_type,
            class future_options,
            class Executor,
            class Function,
            class... Args>
        std::shared_ptr<shared_state<value_type, future_options>>
        make_initial_shared_state(
            const Executor& ex,
            Function&& f,
            Args&&... args) const {
            if constexpr (!future_options::is_always_deferred) {
                using shared_state_t = shared_state<value_type, future_options>;
                (void) f;
                return std::make_shared<shared_state_t>(ex);
            } else {
                using shared_state_t = deferred_shared_state<
                    value_type,
                    future_options,
                    Function,
                    Args...>;
                auto targs = std::make_tuple(std::forward<Args>(args)...);
                return std::make_shared<shared_state_t>(
                    ex,
                    std::forward<Function>(f),
                    std::move(targs));
            }
        }

        /// \brief Schedule the function in the executor
        /// This is the internal function async uses to finally schedule the
        /// function after setting the default parameters and converting
        /// policies into scheduling strategies.
        template <
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
        operator()(const Executor& ex, Function&& f, Args&&... args) const {
            // Function traits
            static constexpr bool is_stoppable = std::
                is_invocable_v<std::decay_t<Function>, stop_token, Args...>;
            using value_type = launch_result_t<Function, Args...>;

            // Future options
            using future_options_a
                = future_options<executor_opt<Executor>, continuable_opt>;
            using future_options_b = conditional_append_future_option_t<
                is_stoppable,
                stoppable_opt,
                future_options_a>;
            using future_options = conditional_append_future_option_t<
                !is_eager,
                always_deferred_opt,
                future_options_b>;

            // Create shared state
            auto state = make_initial_shared_state<value_type, future_options>(
                ex,
                std::forward<Function>(f),
                std::forward<Args>(args)...);
            basic_future<value_type, future_options> fut(state);

            if constexpr (is_eager) {
                // Launch task to fulfill the eager promise now
                asio::post(
                    ex,
                    std::move(
                        [state = std::move(state),
                         f = std::forward<Function>(f),
                         args = std::make_tuple(
                             std::forward<Args>(args)...)]() mutable {
                    state->apply_tuple(std::move(f), std::move(args));
                    }));
            }
            return fut;
        }
    };

    template <bool is_eager>
    constexpr async_future_scheduler<is_eager> schedule_future;

} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_FUTURE_LAUNCHER_HPP
