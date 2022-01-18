//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_IS_EXECUTOR_THEN_FUNCTION_H
#define FUTURES_IS_EXECUTOR_THEN_FUNCTION_H

#include <futures/config/asio_include.h>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    class stop_token;
}

namespace futures::detail {
    /// \brief Check if types are an executor then a function
    /// The function should be invocable with the given args, the executor be an executor, and vice-versa
    template <class E, class F, typename... Args>
    using is_executor_then_function =
        std::conjunction<asio::is_executor<E>, std::negation<asio::is_executor<F>>,
                         std::negation<std::is_invocable<E, Args...>>, std::is_invocable<F, Args...>>;

    template <class E, class F, typename... Args>
    constexpr bool is_executor_then_function_v = is_executor_then_function<E, F, Args...>::value;

    template <class E, class F, typename... Args>
    using is_executor_then_stoppable_function =
        std::conjunction<asio::is_executor<E>, std::negation<asio::is_executor<F>>,
                         std::negation<std::is_invocable<E, stop_token, Args...>>,
                         std::is_invocable<F, stop_token, Args...>>;

    template <class E, class F, typename... Args>
    constexpr bool is_executor_then_stoppable_function_v = is_executor_then_stoppable_function<E, F, Args...>::value;

    template <class F, typename... Args>
    using is_invocable_non_executor =
        std::conjunction<std::negation<asio::is_executor<F>>, std::is_invocable<F, Args...>>;

    template <class F, typename... Args>
    constexpr bool is_invocable_non_executor_v = is_invocable_non_executor<F, Args...>::value;

    template <class F, typename... Args>
    using is_stoppable_invocable_non_executor =
        std::conjunction<std::negation<asio::is_executor<F>>, std::is_invocable<F, stop_token, Args...>>;

    template <class F, typename... Args>
    constexpr bool is_stoppable_invocable_non_executor_v = is_stoppable_invocable_non_executor<F, Args...>::value;

    template <class F, typename... Args>
    using is_async_input_non_executor =
        std::disjunction<is_invocable_non_executor<F, Args...>, is_stoppable_invocable_non_executor<F, Args...>>;

    template <class F, typename... Args>
    constexpr bool is_async_input_non_executor_v = is_async_input_non_executor<F, Args...>::value;
    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_IS_EXECUTOR_THEN_FUNCTION_H
