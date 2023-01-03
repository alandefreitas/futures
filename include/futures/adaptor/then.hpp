//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_THEN_HPP
#define FUTURES_ADAPTOR_THEN_HPP

/**
 *  @file adaptor/then.hpp
 *  @brief Continuation adaptors
 *
 *  This file defines adaptors to create new futures as continuations to
 *  previous tasks.
 */

#include <futures/config.hpp>
#include <futures/adaptor/bind_executor_to_lambda.hpp>
#include <futures/adaptor/detail/internal_then_functor.hpp>

namespace futures {
    /** @addtogroup adaptors Adaptors
     *
     * \brief Functions to create new futures from existing functions.
     *
     * This module defines functions we can use to create new futures from
     * existing futures. Future adaptors are future types of whose values are
     * dependent on the condition of other future objects.
     *
     *  @{
     */

    namespace detail {
        template <class Future, class Function>
        FUTURES_DETAIL(decltype(auto))
        then_no_exec_impl(
            std::true_type /* has_executor */,
            Future &&before,
            Function &&after) {
            return then(
                before.get_executor(),
                std::forward<Future>(before),
                std::forward<Function>(after));
        }

        template <class Future, class Function>
        FUTURES_DETAIL(decltype(auto))
        then_no_exec_impl(
            std::false_type /* has_executor */,
            Future &&before,
            Function &&after) {
            return then(
                ::futures::make_default_executor(),
                std::forward<Future>(before),
                std::forward<Function>(after));
        }
    } // namespace detail

    /// Schedule a continuation function to a future
    /**
     *  This function creates a continuation that gets executed when the
     *  `before` future is completed. The continuation needs to be invocable
     *  with the return type of the previous future.
     *
     *  This function works for all kinds of futures but behavior depends on the
     *  input:
     *  - If the previous future is continuable, attach the function to the
     *  continuation list
     *  - If the previous future is not continuable (such as std::future), post
     *  to execution with deferred policy. In both cases, the result becomes a
     *  cfuture or jcfuture.
     *
     *  Stop tokens are also propagated:
     *  - If after function expects a stop token:
     *    - If previous future is stoppable and not-shared: return jcfuture with
     *    shared stop source
     *    - Otherwise:                                      return jcfuture with
     *    new stop source
     *  - If after function does not expect a stop token:
     *    - If previous future is stoppable and not-shared: return jcfuture with
     *    shared stop source
     *    - Otherwise:                                      return cfuture with
     *    no stop source
     *
     *  @param before The antecedent future
     *  @param after The continuation callable
     *  @return A continuation to the before future
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <future_like Future, class Function>
    requires(
        !is_executor_v<std::decay_t<Function>>
        && !is_executor_v<std::decay_t<Future>>
        && detail::next_future_traits<
            default_executor_type,
            std::decay_t<Function>,
            std::decay_t<Future>>::is_valid)
#else
    template <
        class Future,
        class Function,
        std::enable_if_t<
            !is_executor_v<std::decay_t<Function>>
                && !is_executor_v<std::decay_t<Future>>
                && is_future_like_v<std::decay_t<Future>>
                && detail::next_future_traits<
                    default_executor_type,
                    std::decay_t<Function>,
                    std::decay_t<Future>>::is_valid,
            int>
        = 0>
#endif
    FUTURES_DETAIL(decltype(auto)) then(Future &&before, Function &&after) {
        return detail::then_no_exec_impl(
            has_executor<std::decay_t<Future>>{},
            std::forward<Future>(before),
            std::forward<Function>(after));
    }

    /// @copydoc then
    /**
     * @param ex The executor
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <executor Executor, class Function, class Future>
    requires(
        !is_executor_v<std::decay_t<Function>>
        && !is_executor_v<std::decay_t<Future>>
        && is_future_like_v<std::decay_t<Future>>
        && detail::next_future_traits<
            Executor,
            std::decay_t<Function>,
            std::decay_t<Future>>::is_valid)
#else
    template <
        class Executor,
        class Function,
        class Future,
        std::enable_if_t<
            (is_executor_v<std::decay_t<Executor>>
             && !is_executor_v<std::decay_t<Function>>
             && !is_executor_v<std::decay_t<Future>>
             && is_future_like_v<std::decay_t<Future>>
             && detail::next_future_traits<
                 Executor,
                 std::decay_t<Function>,
                 std::decay_t<Future>>::is_valid),
            int>
        = 0>
#endif
    FUTURES_DETAIL(decltype(auto))
        then(Executor const &ex, Future &&before, Function &&after) {
        return detail::internal_then(
            ex,
            std::forward<Future>(before),
            std::forward<Function>(after));
    }

    /// Operator to schedule a continuation function to a future
    ///
    /// @return A continuation to the before future
#ifdef FUTURES_HAS_CONCEPTS
    template <future_like Future, class Function>
    requires(
        !is_executor_v<std::decay_t<Function>>
        && !is_executor_v<std::decay_t<Future>>
        && detail::next_future_traits<
            default_executor_type,
            std::decay_t<Function>,
            std::decay_t<Future>>::is_valid)
#else
    template <
        class Future,
        class Function,
        std::enable_if_t<
            !is_executor_v<std::decay_t<Function>>
                && !is_executor_v<std::decay_t<Future>>
                && is_future_like_v<std::decay_t<Future>>
                && detail::next_future_traits<
                    default_executor_type,
                    std::decay_t<Function>,
                    std::decay_t<Future>>::is_valid,
            int>
        = 0>
#endif
    FUTURES_DETAIL(decltype(auto))
    operator>>(Future &&before, Function &&after) {
        return then(std::forward<Future>(before), std::forward<Function>(after));
    }

    /// @copydoc then
#ifdef FUTURES_HAS_CONCEPTS
    template <class Executor, class Future, class Function, bool RValue>
    requires(
        is_executor_v<std::decay_t<Executor>>
        && !is_executor_v<std::decay_t<Function>>
        && !is_executor_v<std::decay_t<Future>>
        && is_future_like_v<std::decay_t<Future>>
        && detail::next_future_traits<
            Executor,
            std::decay_t<Function>,
            std::decay_t<Future>>::is_valid)
#else
    template <
        class Executor,
        class Future,
        class Function,
        bool RValue,
        std::enable_if_t<
            (is_executor_v<std::decay_t<Executor>>
             && !is_executor_v<std::decay_t<Function>>
             && !is_executor_v<std::decay_t<Future>>
             && is_future_like_v<std::decay_t<Future>>
             && detail::next_future_traits<
                 Executor,
                 std::decay_t<Function>,
                 std::decay_t<Future>>::is_valid),
            int>
        = 0>
#endif
    FUTURES_DETAIL(decltype(auto))
    operator>>(
        Future &&before,
        detail::executor_and_callable_reference<Executor, Function, RValue>
            &&after) {
        return then(
            after.get_executor(),
            std::forward<Future>(before),
            std::forward<Function>(after.get_callable()));
    }

    /** @} */
} // namespace futures

#endif // FUTURES_ADAPTOR_THEN_HPP
