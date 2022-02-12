//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_BIND_EXECUTOR_TO_LAMBDA_HPP
#define FUTURES_ADAPTOR_BIND_EXECUTOR_TO_LAMBDA_HPP

#include <futures/detail/traits/is_callable.hpp>
#include <futures/executor/is_executor.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup adaptors Adaptors
     *  @{
     */

    namespace detail {
        template <class Executor, class Function, bool RValue>
        struct executor_and_callable_reference
        {
            std::reference_wrapper<const Executor> ex;
            std::reference_wrapper<Function> fn;

            constexpr const Executor&
            get_executor() noexcept {
                return ex;
            }

            constexpr auto
            get_callable() noexcept {
                if constexpr (!RValue) {
                    return fn;
                } else {
                    return static_cast<
                        typename std::remove_reference<Function>::type&&>(fn);
                }
            }
        };
    } // namespace detail

    /// Create a proxy pair with a lambda and an executor
    /**
     * For this operation, we needed an operator with higher precedence than
     * operator>> Our options are: +, -, *, /, %, &, !, ~. Although + seems
     * like an obvious choice, % is the one that leads to less conflict with
     * other functions.
     *
     * @return A proxy pair to schedule execution
     */
    template <
        class Executor,
        typename Function,
        typename... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            is_executor_v<std::decay_t<Executor>> &&
            detail::is_callable_v<std::decay_t<Function>>
            // clang-format on
            ,
            int> = 0
#endif
        >
    decltype(auto)
    operator%(const Executor& ex, Function&& after) {
        return detail::executor_and_callable_reference<
            std::decay_t<Executor>,
            std::decay_t<Function>,
            std::is_rvalue_reference_v<Function>>{
            std::cref(ex),
            std::ref(after)
        };
    }

    /** @} */

} // namespace futures

#endif // FUTURES_ADAPTOR_BIND_EXECUTOR_TO_LAMBDA_HPP
