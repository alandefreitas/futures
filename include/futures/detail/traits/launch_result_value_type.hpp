//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_LAUNCH_RESULT_VALUE_TYPE_HPP
#define FUTURES_DETAIL_TRAITS_LAUNCH_RESULT_VALUE_TYPE_HPP

#include <futures/stop_token.hpp>
#include <futures/detail/traits/type_member_or_void.hpp>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */

    /// The return type of a callable given to futures::async with the
    /// Args... This is the value type of the future object returned by async.
    /// In typical implementations this is usually the same as
    /// result_of_t<Function, Args...>. However, our implementation is a little
    /// different as the stop_token is provided by the async function and is
    /// thus not a part of Args, so both paths need to be considered.
    template <typename Function, typename... Args>
    using launch_result_value_type = std::conditional<
        std::is_invocable_v<std::decay_t<Function>, stop_token, Args...>,
        type_member_or_void_t<
            std::invoke_result<std::decay_t<Function>, stop_token, Args...>>,
        type_member_or_void_t<
            std::invoke_result<std::decay_t<Function>, Args...>>>;

    template <typename Function, typename... Args>
    using launch_result_value_type_t =
        typename launch_result_value_type<Function, Args...>::type;

    /** @} */
} // namespace futures::detail


#endif // FUTURES_DETAIL_TRAITS_LAUNCH_RESULT_VALUE_TYPE_HPP
