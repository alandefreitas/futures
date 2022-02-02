//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURE_OPTIONS_HPP
#define FUTURES_FUTURE_OPTIONS_HPP

#include <futures/futures/detail/traits/get_type_template_in_args.hpp>
#include <futures/futures/detail/traits/is_in_args.hpp>
#include <futures/futures/detail/traits/is_type_template_in_args.hpp>
#include <futures/executor/default_executor.hpp>
#include <type_traits>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-types Future types
     *  @{
     */

    struct continuable_opt
    {};

    struct stoppable_opt
    {};

    struct always_detached_opt
    {};

    struct deferred_opt
    {};

    template <class Fn>
    struct executor_opt
    {
        using type = Fn;
    };

    struct shared_opt
    {};

    template <class... Args>
    struct future_options
    {
        static constexpr bool has_executor = detail::
            is_type_template_in_args_v<executor_opt, Args...>;

        using executor_t = detail::get_type_template_in_args_t<
            default_executor_type,
            executor_opt,
            Args...>;

        static constexpr bool is_continuable = detail::
            is_in_args_v<continuable_opt, Args...>;

        static constexpr bool is_stoppable = detail::
            is_in_args_v<stoppable_opt, Args...>;

        static constexpr bool is_always_detached = detail::
            is_in_args_v<always_detached_opt, Args...>;

        static constexpr bool is_deferred = detail::
            is_in_args_v<deferred_opt, Args...>;

        static constexpr bool is_shared = detail::
            is_in_args_v<shared_opt, Args...>;
    };

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURE_OPTIONS_HPP
