//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_FUTURE_CONTINUE_TASK_HPP
#define FUTURES_ADAPTOR_DETAIL_FUTURE_CONTINUE_TASK_HPP

#include <futures/config.hpp>
#include <futures/adaptor/detail/continue.hpp>
#include <utility>
#include <type_traits>

namespace futures::detail {
    constexpr future_continue_functor future_continue;

    // A functor that stores both future and the function for the continuation
    template <class Future, class Function>
    struct future_continue_task {
        Future before_;
        Function after_;

        continue_invoke_result_t<Future, Function>
        operator()() {
            return future_continue(std::move(before_), std::move(after_));
        }

        mp_eval_if<
            continue_is_invocable<Future, Function>,
            detail::continue_tags::failure,
            continue_invoke_result_t,
            Future,
            Function,
            stop_token>
        operator()(stop_token st) {
            return future_continue(std::move(before_), std::move(after_), st);
        }
    };

    // Identify future_continue_task in case the shared state need to knows
    // how to handle it
    template <class Function>
    struct is_future_continue_task : std::false_type {};

    template <class Future, class Function>
    struct is_future_continue_task<future_continue_task<Future, Function>>
        : std::true_type {};

} // namespace futures::detail

#endif // FUTURES_ADAPTOR_DETAIL_FUTURE_CONTINUE_TASK_HPP
