//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_LAUNCH_HPP
#define FUTURES_IMPL_LAUNCH_HPP

namespace futures {
    FUTURES_TEMPLATE_IMPL(class Executor, class Function, class... Args)
    (requires(
        is_executor_v<Executor>
        && (std::is_invocable_v<Function, Args...>
            || std::is_invocable_v<Function, stop_token, Args...>) ))
        FUTURES_DETAIL(decltype(auto))
            async(Executor const &ex, Function &&f, Args &&...args) {
        return detail::async_future_scheduler{}
            .schedule<
                detail::async_future_options_t<Executor, Function, Args...>>(
                ex,
                std::forward<Function>(f),
                std::forward<Args>(args)...);
    }

    FUTURES_TEMPLATE_IMPL(class Function, class... Args)
    (requires(
        !is_executor_v<Function>
        && (std::is_invocable_v<Function, Args...>
            || std::is_invocable_v<Function, stop_token, Args...>) ))
        FUTURES_DETAIL(decltype(auto)) async(Function &&f, Args &&...args) {
        return detail::async_future_scheduler{}
            .schedule<detail::async_future_options_t<
                default_executor_type,
                Function,
                Args...>>(
                ::futures::make_default_executor(),
                std::forward<Function>(f),
                std::forward<Args>(args)...);
    }

    FUTURES_TEMPLATE_IMPL(class Executor, class Function, class... Args)
    (requires(
        (is_executor_v<Executor>
         && (std::is_invocable_v<Function, Args...>
             || std::is_invocable_v<Function, stop_token, Args...>) )))
        FUTURES_DETAIL(decltype(auto))
            schedule(Executor const &ex, Function &&f, Args &&...args) {
        return detail::async_future_scheduler{}
            .schedule<
                detail::schedule_future_options_t<Executor, Function, Args...>>(
                ex,
                std::forward<Function>(f),
                std::forward<Args>(args)...);
    }

    FUTURES_TEMPLATE_IMPL(class Function, class... Args)
    (requires(
        (!is_executor_v<Function>
         && (std::is_invocable_v<Function, Args...>
             || std::is_invocable_v<Function, stop_token, Args...>) )))
        FUTURES_DETAIL(decltype(auto)) schedule(Function &&f, Args &&...args) {
        return detail::async_future_scheduler{}
            .schedule<detail::schedule_future_options_t<
                default_executor_type,
                Function,
                Args...>>(
                ::futures::make_default_executor(),
                std::forward<Function>(f),
                std::forward<Args>(args)...);
    }
} // namespace futures

#endif // FUTURES_IMPL_LAUNCH_HPP