//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_BIND_EXECUTOR_TO_LAMBDA_HPP
#define FUTURES_ADAPTOR_BIND_EXECUTOR_TO_LAMBDA_HPP

#include <futures/executor/is_executor.hpp>
#include <futures/detail/traits/is_callable.hpp>
#include <futures/detail/deps/boost/mp11/integral.hpp>
#include <functional>
#include <type_traits>

/**
 *  @file adaptor/bind_executor_to_lambda.hpp
 *  @brief Attach executor to callable
 *
 *  This file defines the operator we can use to bind an executor to a
 *  callable. This is an intermediary step for executors.
 */

namespace futures {
    namespace detail {
        template <class Executor, class Function, bool RValue>
        struct executor_and_callable_reference {
            std::reference_wrapper<Executor const> ex;
            std::reference_wrapper<Function> fn;

            constexpr Executor const&
            get_executor() noexcept {
                return ex;
            }

            constexpr auto
            get_callable() noexcept {
                return get_callable_impl(detail::mp_bool<RValue>{});
            }

        private:
            constexpr auto
            get_callable_impl(std::true_type /* RValue */) noexcept {
                return static_cast<
                    typename std::remove_reference<Function>::type&&>(fn);
            }

            constexpr auto
            get_callable_impl(std::false_type /* RValue */) noexcept {
                return fn;
            }
        };
    } // namespace detail

    /** @addtogroup adaptors Adaptors
     *  @{
     */

    /// Create a proxy pair with a lambda and an executor
    /**
     * For this operation, we needed an operator with higher precedence than
     * operator>> Our options are: +, -, *, /, %, &, !, ~. Although + seems
     * like an obvious choice, % is the one that leads to less conflict with
     * other functions.
     *
     * @param ex An executor
     * @param after A callable with the continuation
     * @return A proxy pair to schedule execution
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <executor Executor, class Function>
    requires detail::is_callable_v<std::decay_t<Function>>
#else
    template <
        class Executor,
        class Function,
        std::enable_if_t<
            is_executor_v<std::decay_t<Executor>>
                && detail::is_callable_v<std::decay_t<Function>>,
            int>
        = 0>
#endif
    FUTURES_DETAIL(decltype(auto))
    operator%(Executor const& ex, Function&& after) {
        return detail::executor_and_callable_reference<
            std::decay_t<Executor>,
            std::decay_t<Function>,
            std::is_rvalue_reference<Function>::value>{
            std::cref(ex),
            std::ref(after)
        };
    }

    /** @} */

} // namespace futures

#endif // FUTURES_ADAPTOR_BIND_EXECUTOR_TO_LAMBDA_HPP
