//
// Created by Alan Freitas on 8/18/21.
//

#ifndef CPP_MANIFEST_IS_EXECUTOR_THEN_CONTINUATION_H
#define CPP_MANIFEST_IS_EXECUTOR_THEN_CONTINUATION_H

#include <asio.hpp>

#include "is_future.h"
#include "is_future_continuation.h"
#include "unwrap_future.h"

namespace futures::detail {
    /// This is very similar to is_executor_then_function, but receives the "before" future type
    /// instead of the function arguments. Besides checking if something is not an executor,
    /// it checks if one function can be a continuation to a future. After that, we also need
    /// to consider the case where this is a future<void>, so that the function should be
    /// invocable with no arguments instead of a void argument.

    /// Check if (i) E is an executor, (ii) Function is a function, which can be (iii) a continuation for Future
    template <class Executor, class Function, class Future>
    using is_executor_then_continuation =
        std::conjunction<asio::is_executor<Executor>, std::negation<asio::is_executor<Function>>,
                         std::negation<asio::is_executor<Future>>, is_future<Future>,
                         std::disjunction<is_future_continuation<Function, Future>,
                                          is_future_continuation<Function, Future, stop_token>>>;

    template <class Executor, class Function, class Future>
    constexpr bool is_executor_then_continuation_v = is_executor_then_continuation<Executor, Function, Future>::value;

    template <class Function, class Future>
    using is_continuation_non_executor =
        std::conjunction<std::negation<asio::is_executor<Function>>, std::negation<asio::is_executor<Future>>,
                         is_future<Future>,
                         std::disjunction<is_future_continuation<Function, Future>,
                                          is_future_continuation<Function, Future, stop_token>>>;

    template <class Function, class Future>
    constexpr bool is_continuation_non_executor_v = is_continuation_non_executor<Function, Future>::value;
} // namespace futures::detail

#endif // CPP_MANIFEST_IS_EXECUTOR_THEN_CONTINUATION_H
