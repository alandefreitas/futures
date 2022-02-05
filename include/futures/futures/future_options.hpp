//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_FUTURE_OPTIONS_HPP
#define FUTURES_FUTURES_FUTURE_OPTIONS_HPP

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

    struct always_deferred_opt
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
        /// \brief Whether the future has an associated executor
        static constexpr bool has_executor = detail::
            is_type_template_in_args_v<executor_opt, Args...>;

        /// \brief Executor used by the shared state
        ///
        /// This is the executor the shared state is using for the
        /// current task and the default executor it uses for
        /// potential continuations
        using executor_t = detail::get_type_template_in_args_t<
            default_executor_type,
            executor_opt,
            Args...>;

        /// \brief Whether the future supports deferred continuations
        static constexpr bool is_continuable = detail::
            is_in_args_v<continuable_opt, Args...>;

        /// \brief Whether the future supports stop requests
        static constexpr bool is_stoppable = detail::
            is_in_args_v<stoppable_opt, Args...>;

        /// \brief Whether the future is always detached
        static constexpr bool is_always_detached = detail::
            is_in_args_v<always_detached_opt, Args...>;

        /// \brief Whether the future is always deferred
        ///
        /// Deferred futures are associated to a task that is only
        /// sent to the executor when we request or wait for the
        /// future value
        static constexpr bool is_always_deferred = detail::
            is_in_args_v<always_deferred_opt, Args...>;

        /// \brief Whether the future is shared
        ///
        /// The value of shared futures is not consumed when requested.
        /// Instead, it makes copies of the return value. On the other
        /// hand, simple unique future move their result from the
        /// shared state when their value is requested.
        static constexpr bool is_shared = detail::
            is_in_args_v<shared_opt, Args...>;
    };

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURES_FUTURE_OPTIONS_HPP
