//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_LAUNCH_HPP
#define FUTURES_DETAIL_LAUNCH_HPP

#include <futures/config.hpp>
#include <futures/future_options.hpp>
#include <futures/stop_token.hpp>
#include <futures/detail/operation_state.hpp>
#include <futures/detail/traits/is_future_options.hpp>
#include <futures/detail/deps/boost/pool/pool_alloc.hpp>
#include <type_traits>

namespace futures {
    namespace detail {
        // The allocator we use by default for new eager futures
        /*
         * One of the most important reasons why eager futures are slower
         * than deferred futures is dynamic memory allocation. These pool
         * allocators mitigate that cost.
         */
        template <class T>
        using default_futures_allocator = boost::fast_pool_allocator<T>;

        // Determine the options for a basic_future returned from async
        template <class Executor, class Function, class... Args>
        struct async_future_options {
            using type = conditional_append_future_option_t<
                is_invocable_v<std::decay_t<Function>, stop_token, Args...>,
                stoppable_opt,
                future_options<executor_opt<Executor>, continuable_opt>>;
            FUTURES_STATIC_ASSERT(is_future_options_v<type>);
        };

        template <class Executor, class Function, class... Args>
        using async_future_options_t =
            typename async_future_options<Executor, Function, Args...>::type;

        // Determine the options for a basic_future returned from schedule
        template <class Executor, class Function, class... Args>
        struct schedule_future_options {
        private:
            // base options
            using base_options = future_options<executor_opt<Executor>>;
            FUTURES_STATIC_ASSERT(is_future_options_v<base_options>);

            // maybe include stop token
            using maybe_stoppable_options = conditional_append_future_option_t<
                is_invocable_v<std::decay_t<Function>, stop_token, Args...>,
                stoppable_opt,
                future_options<executor_opt<Executor>>>;
            FUTURES_STATIC_ASSERT(is_future_options_v<maybe_stoppable_options>);

            // include deferred option
            using deferred_options = append_future_option_t<
                always_deferred_opt,
                maybe_stoppable_options>;
            FUTURES_STATIC_ASSERT(is_future_options_v<deferred_options>);

            // include deferred function type
            using typed_deferred_function = std::conditional_t<
                sizeof...(Args) == 0,
                Function,
                bind_deferred_state_args<Function, Args...>>;
            FUTURES_STATIC_ASSERT(is_invocable_v<typed_deferred_function>);
            using typed_deferred_options = append_future_option_t<
                deferred_function_opt<typed_deferred_function>,
                deferred_options>;
            FUTURES_STATIC_ASSERT(is_future_options_v<typed_deferred_options>);
        public:
            using type = typed_deferred_options;
            FUTURES_STATIC_ASSERT(is_future_options_v<type>);
        };

        template <class Executor, class Function, class... Args>
        using schedule_future_options_t =
            typename schedule_future_options<Executor, Function, Args...>::type;
    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_LAUNCH_HPP
