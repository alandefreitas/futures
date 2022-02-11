//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_THEN_HPP
#define FUTURES_ADAPTOR_THEN_HPP

#include <futures/adaptor/detail/internal_then_functor.hpp>
#include <future>
#include <version>

namespace futures {
    /** \addtogroup adaptors Adaptors
     *
     * \brief Functions to create new futures from existing functions.
     *
     * This module defines functions we can use to create new futures from
     * existing futures. Future adaptors are future types of whose values are
     * dependant on the condition of other future objects.
     *
     *  @{
     */

    /// \brief Schedule a continuation function to a future
    ///
    /// This creates a continuation that gets executed when the before future is
    /// over. The continuation needs to be invocable with the return type of the
    /// previous future.
    ///
    /// This function works for all kinds of futures but behavior depends on the
    /// input:
    /// - If previous future is continuable, attach the function to the
    /// continuation list
    /// - If previous future is not continuable (such as std::future), post to
    /// execution with deferred policy In both cases, the result becomes a
    /// cfuture or jcfuture.
    ///
    /// Stop tokens are also propagated:
    /// - If after function expects a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with
    ///   shared stop source
    ///   - Otherwise:                                      return jcfuture with
    ///   new stop source
    /// - If after function does not expect a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with
    ///   shared stop source
    ///   - Otherwise:                                      return cfuture with
    ///   no stop source
    ///
    /// \return A continuation to the before future
    template <
        typename Executor,
        typename Function,
        class Future
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            is_executor_v<std::decay_t<Executor>> &&
            !is_executor_v<std::decay_t<Function>> &&
            !is_executor_v<std::decay_t<Future>> &&
            is_future_v<std::decay_t<Future>> &&
            detail::continuation_traits<Executor, std::decay_t<Function>, std::decay_t<Future>>::is_valid,
            // clang-format on
            int> = 0
#endif
        >
    decltype(auto)
    then(const Executor &ex, Future &&before, Function &&after) {
        return detail::internal_then(
            ex,
            std::forward<Future>(before),
            std::forward<Function>(after));
    }

    /// \brief Schedule a continuation function to a future with the default
    /// executor
    ///
    /// \return A continuation to the before future
    ///
    /// \see @ref then
    template <
        class Future,
        typename Function
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            !is_executor_v<std::decay_t<Function>> &&
            !is_executor_v<std::decay_t<Future>> &&
            is_future_v<std::decay_t<Future>> &&
            detail::continuation_traits<default_executor_type, std::decay_t<Function>, std::decay_t<Future>>::is_valid
            // clang-format on
            ,
            int> = 0
#endif
        >
    decltype(auto)
    then(Future &&before, Function &&after) {
        if constexpr (has_executor_v<std::decay_t<Future>>) {
            return then(
                before.get_executor(),
                std::forward<Future>(before),
                std::forward<Function>(after));
        } else {
            return then(
                ::futures::make_default_executor(),
                std::forward<Future>(before),
                std::forward<Function>(after));
        }
    }

    /// \brief Operator to schedule a continuation function to a future
    ///
    /// \return A continuation to the before future
    template <
        class Future,
        typename Function
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            !is_executor_v<std::decay_t<Function>> &&
            !is_executor_v<std::decay_t<Future>> &&
            is_future_v<std::decay_t<Future>> &&
            detail::continuation_traits<default_executor_type, std::decay_t<Function>, std::decay_t<Future>>::is_valid
            // clang-format on
            ,
            int> = 0
#endif
        >
    decltype(auto)
    operator>>(Future &&before, Function &&after) {
        return then(std::forward<Future>(before), std::forward<Function>(after));
    }

    /// \brief Schedule a continuation function to a future with a custom
    /// executor
    ///
    /// \return A continuation to the before future
    template <
        class Executor,
        class Future,
        typename Function
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            is_executor_v<std::decay_t<Executor>> &&
            !is_executor_v<std::decay_t<Function>> &&
            !is_executor_v<std::decay_t<Future>> &&
            is_future_v<std::decay_t<Future>> &&
            detail::continuation_traits<Executor, std::decay_t<Function>, std::decay_t<Future>>::is_valid
            // clang-format on
            ,
            int> = 0
#endif
        >
    decltype(auto)
    operator>>(
        Future &&before,
        std::pair<const Executor &, Function &> &&after) {
        return then(
            after.first,
            std::forward<Future>(before),
            std::forward<Function>(after.second));
    }

    /// \brief Create a proxy pair to schedule a continuation function to a
    /// future with a custom executor
    ///
    /// For this operation, we needed an operator with higher precedence than
    /// operator>> Our options are: +, -, *, /, %, &, !, ~. Although + seems
    /// like an obvious choice, % is the one that leads to less conflict with
    /// other functions.
    ///
    /// \return A proxy pair to schedule execution
    template <
        class Executor,
        typename Function,
        typename... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            is_executor_v<std::decay_t<
                Executor>> && !is_executor_v<std::decay_t<Function>> && !is_callable_v<std::decay_t<Executor>> && is_callable_v<std::decay_t<Function>>,
            int> = 0
#endif
        >
    decltype(auto)
    operator%(const Executor &ex, Function &&after) {
        return std::make_pair(std::cref(ex), std::ref(after));
    }

    /** @} */
} // namespace futures

#endif // FUTURES_ADAPTOR_THEN_HPP
