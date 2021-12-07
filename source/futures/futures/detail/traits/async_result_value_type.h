//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_ASYNC_RESULT_VALUE_TYPE_H
#define FUTURES_ASYNC_RESULT_VALUE_TYPE_H

#include <futures/futures/detail/traits/type_member_or_void.h>
#include <futures/futures/stop_token.h>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief The return type of a callable given to futures::async with the Args...
    /// This is the value type of the future object returned by async.
    /// In typical implementations this is usually the same as result_of_t<Function, Args...>.
    /// However, our implementation is a little different as the stop_token is provided by the
    /// async function and is thus not a part of Args, so both paths need to be considered.
    template <typename Function, typename... Args>
    using async_result_value_type =
        std::conditional<std::is_invocable_v<std::decay_t<Function>, stop_token, Args...>,
                           type_member_or_void_t<std::invoke_result<std::decay_t<Function>, stop_token, Args...>>,
                           type_member_or_void_t<std::invoke_result<std::decay_t<Function>, Args...>>>;

    template <typename Function, typename... Args>
    using async_result_value_type_t = typename async_result_value_type<Function, Args...>::type;

    /** @} */
}


#endif // FUTURES_ASYNC_RESULT_VALUE_TYPE_H
