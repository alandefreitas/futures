//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_FUTURE_OPTIONS_SET_HPP
#define FUTURES_FUTURES_DETAIL_FUTURE_OPTIONS_SET_HPP

#include <futures/futures/detail/future_options_list.hpp>
#include <futures/futures/detail/traits/append_future_option.hpp>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-options Future options
     *  @{
     */

    /// Future options list as a sorted set
    template <class... Args>
    struct future_options_flat
    {
    private:
        using empty_opts_type = future_options_list<>;
        using maybe_executor_list_t = conditional_append_future_option_t<
            is_type_template_in_args_v<executor_opt, Args...>,
            executor_opt<get_type_template_in_args_t<
                default_executor_type,
                executor_opt,
                Args...>>,
            empty_opts_type>;
        using maybe_continuable_list_t = conditional_append_future_option_t<
            is_in_args_v<continuable_opt, Args...>,
            continuable_opt,
            maybe_executor_list_t>;
        using maybe_stoppable_list_t = conditional_append_future_option_t<
            is_in_args_v<stoppable_opt, Args...>,
            stoppable_opt,
            maybe_continuable_list_t>;
        using maybe_detached_list_t = conditional_append_future_option_t<
            is_in_args_v<always_detached_opt, Args...>,
            always_detached_opt,
            maybe_stoppable_list_t>;
        using maybe_deferred_list_t = conditional_append_future_option_t<
            is_in_args_v<always_deferred_opt, Args...>,
            always_deferred_opt,
            maybe_detached_list_t>;
        using maybe_shared_list_t = conditional_append_future_option_t<
            is_in_args_v<shared_opt, Args...>,
            shared_opt,
            maybe_deferred_list_t>;
    public:
        using type = maybe_shared_list_t;
    };

    template <class... Args>
    using future_options_flat_t = typename future_options_flat<Args...>::type;

} // namespace futures::detail


#endif // FUTURES_FUTURES_DETAIL_FUTURE_OPTIONS_SET_HPP
