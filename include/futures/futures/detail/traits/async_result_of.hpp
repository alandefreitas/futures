//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ASYNC_RESULT_OF_H
#define FUTURES_ASYNC_RESULT_OF_H

#include <futures/futures/basic_future.hpp>
#include <futures/futures/detail/traits/async_result_value_type.hpp>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief The future type that results from calling async with a function
    /// <Function, Args...> This is the future type returned by async. In
    /// typical implementations this is usually the same as
    /// future<result_of_t<Function, Args...>>. However, our implementation is a
    /// little different as the stop_token is provided by the async function and
    /// can thus influence the resulting future type, so both paths need to be
    /// considered. Whenever we call async, we return a future with lazy
    /// continuations by default because we don't know if the user will need
    /// efficient continuations. Also, when the function expects a stop token,
    /// we return a jfuture.
    template <typename Function, typename... Args>
    using async_result_of = std::conditional<
        std::is_invocable_v<std::decay_t<Function>, stop_token, Args...>,
        jcfuture<async_result_value_type_t<Function, Args...>>,
        cfuture<async_result_value_type_t<Function, Args...>>>;

    template <typename Function, typename... Args>
    using async_result_of_t = typename async_result_of<Function, Args...>::type;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_ASYNC_RESULT_OF_H