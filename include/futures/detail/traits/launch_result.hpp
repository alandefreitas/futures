//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_LAUNCH_RESULT_HPP
#define FUTURES_DETAIL_TRAITS_LAUNCH_RESULT_HPP

#include <futures/basic_future.hpp>
#include <futures/detail/traits/launch_result_value_type.hpp>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */

    template <class Function, class = void, class... Args>
    struct launch_result_impl
    {};

    template <class Function, class... Args>
    struct launch_result_impl<
        Function,
        std::enable_if_t<std::is_invocable_v<std::decay_t<Function>, Args...>>,
        Args...>
    {
        using type = std::invoke_result_t<std::decay_t<Function>, Args...>;
    };

    template <class Function, class... Args>
    struct launch_result_impl<
        Function,
        std::enable_if_t<
            std::is_invocable_v<std::decay_t<Function>, stop_token, Args...>>,
        Args...>
    {
        using type = std::
            invoke_result_t<std::decay_t<Function>, stop_token, Args...>;
    };

    /// The future type that results from calling async with a function
    ///
    /// This is the future type returned by async and schedule.
    ///
    /// In typical implementations this is usually the same as
    /// future<result_of_t<Function, Args...>>. However, our implementation
    /// extends that as a stop_token might need to be provided by the async
    /// function and can thus influence the resulting future type.
    ///
    /// Thus, both paths need to be considered. Whenever we call async, we
    /// return a future with lazy continuations by default because we don't
    /// know if the user will need efficient continuations, even though
    /// this is expected more often than not in asynchronous applications.
    ///
    /// When the function expects a stop token, we return a jcfuture, because
    /// it has needs a token and continuations.
    template <class Function, class... Args>
    using launch_result = launch_result_impl<Function, void, Args...>;

    template <class Function, class... Args>
    using launch_result_t = typename launch_result<Function, Args...>::type;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_LAUNCH_RESULT_HPP
