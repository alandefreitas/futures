//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_FUTURE_OPTIONS_SET_HPP
#define FUTURES_DETAIL_FUTURE_OPTIONS_SET_HPP

#include <futures/detail/future_options_list.hpp>
#include <futures/detail/traits/append_future_option.hpp>

namespace futures {
    namespace detail {
        /** @addtogroup futures Futures
         *  @{
         */
        /** @addtogroup future-options Future options
         *  @{
         */

        /// Future options list as a sorted set
        template <class... Args>
        struct future_options_flat {
        private:
            static constexpr std::size_t N = sizeof...(Args);

            template <class T>
            struct is_executor_opt {
                static constexpr bool value = false;
            };

            template <class T>
            struct is_executor_opt<executor_opt<T>> {
                static constexpr bool value = true;
            };

            template <class TypeList>
            using get_executor_opt_type = typename mp_at<
                TypeList,
                mp_find_if<TypeList, is_executor_opt>>::type;

            using empty_opts_type = future_options_list<>;

            using maybe_executor_list_t = conditional_append_future_option_t<
                mp_find_if<mp_list<Args...>, is_executor_opt>::value != N,
                executor_opt<mp_eval_or<
                    default_executor_type,
                    get_executor_opt_type,
                    mp_list<Args...>>>,
                empty_opts_type>;

            using maybe_continuable_list_t = conditional_append_future_option_t<
                mp_contains<mp_list<Args...>, continuable_opt>::value,
                continuable_opt,
                maybe_executor_list_t>;

            using maybe_stoppable_list_t = conditional_append_future_option_t<
                mp_contains<mp_list<Args...>, stoppable_opt>::value,
                stoppable_opt,
                maybe_continuable_list_t>;

            using maybe_detached_list_t = conditional_append_future_option_t<
                mp_contains<mp_list<Args...>, always_detached_opt>::value,
                always_detached_opt,
                maybe_stoppable_list_t>;

            using maybe_deferred_list_t = conditional_append_future_option_t<
                mp_contains<mp_list<Args...>, always_deferred_opt>::value,
                always_deferred_opt,
                maybe_detached_list_t>;

            using maybe_shared_list_t = conditional_append_future_option_t<
                mp_contains<mp_list<Args...>, shared_opt>::value,
                shared_opt,
                maybe_deferred_list_t>;

        public:
            using type = maybe_shared_list_t;
        };

        template <class... Args>
        using future_options_flat_t = typename future_options_flat<
            Args...>::type;

    } // namespace detail
} // namespace futures


#endif // FUTURES_DETAIL_FUTURE_OPTIONS_SET_HPP
