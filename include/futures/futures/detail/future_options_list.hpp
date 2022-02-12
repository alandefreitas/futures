//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_FUTURE_OPTIONS_LIST_HPP
#define FUTURES_FUTURES_DETAIL_FUTURE_OPTIONS_LIST_HPP

#include <futures/executor/default_executor.hpp>
#include <futures/futures/detail/traits/get_type_template_in_args.hpp>
#include <futures/futures/detail/traits/index_in_args.hpp>
#include <futures/futures/detail/traits/is_in_args.hpp>
#include <futures/futures/detail/traits/is_type_template_in_args.hpp>
#include <type_traits>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-options Future options
     *  @{
     */

    /// Class used to define future extension at compile-time
    template <class... Args>
    struct future_options_list
    {
        /// Whether the future has an associated executor
        static constexpr bool has_executor = detail::
            is_type_template_in_args_v<executor_opt, Args...>;

        /// Executor used by the shared state
        ///
        /// This is the executor the shared state is using for the
        /// current task and the default executor it uses for
        /// potential continuations
        using executor_t = detail::get_type_template_in_args_t<
            default_executor_type,
            executor_opt,
            Args...>;

        /// Whether the future supports deferred continuations
        static constexpr bool is_continuable = detail::
            is_in_args_v<continuable_opt, Args...>;

        /// Whether the future supports stop requests
        static constexpr bool is_stoppable = detail::
            is_in_args_v<stoppable_opt, Args...>;

        /// Whether the future is always detached
        static constexpr bool is_always_detached = detail::
            is_in_args_v<always_detached_opt, Args...>;

        /// Whether the future is always deferred
        ///
        /// Deferred futures are associated to a task that is only
        /// sent to the executor when we request or wait for the
        /// future value
        static constexpr bool is_always_deferred = detail::
            is_in_args_v<always_deferred_opt, Args...>;

        /// Whether the future is shared
        ///
        /// The value of shared futures is not consumed when requested.
        /// Instead, it makes copies of the return value. On the other
        /// hand, simple unique future move their result from the
        /// shared state when their value is requested.
        static constexpr bool is_shared = detail::
            is_in_args_v<shared_opt, Args...>;

    private:
        // Identify args positions and ensure they are sorted
        // executor_opt < continuable_opt
        static constexpr std::size_t executor_idx
            = index_in_args_v<executor_opt<executor_t>, Args...>;
        static constexpr std::size_t continuable_idx
            = index_in_args_v<continuable_opt, Args...>;
        static_assert(
            executor_idx == std::size_t(-1) || executor_idx < continuable_idx);

        // continuable_opt < stoppable_opt
        static constexpr std::size_t stoppable_idx
            = index_in_args_v<stoppable_opt, Args...>;
        static_assert(
            continuable_idx == std::size_t(-1) || continuable_idx < stoppable_idx);

        // stoppable_opt < always_detached_opt
        static constexpr std::size_t always_detached_idx
            = index_in_args_v<always_detached_opt, Args...>;
        static_assert(
            stoppable_idx == std::size_t(-1) || stoppable_idx < always_detached_idx);

        // always_detached_opt < always_deferred_opt
        static constexpr std::size_t always_deferred_idx
            = index_in_args_v<always_deferred_opt, Args...>;
        static_assert(
            always_detached_idx == std::size_t(-1) || always_detached_idx < always_deferred_idx);

        // always_deferred_opt < shared_opt
        static constexpr std::size_t shared_idx
            = index_in_args_v<shared_opt, Args...>;
        static_assert(
            always_deferred_idx == std::size_t(-1) || always_deferred_idx < shared_idx);
    };

    /** @} */
    /** @} */
} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_FUTURE_OPTIONS_LIST_HPP
