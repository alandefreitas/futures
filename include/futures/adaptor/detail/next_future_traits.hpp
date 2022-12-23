//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_NEXT_FUTURE_TRAITS_HPP
#define FUTURES_ADAPTOR_DETAIL_NEXT_FUTURE_TRAITS_HPP

#include <futures/config.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/adaptor/detail/future_continue_task.hpp>
#include <utility>
#include <type_traits>


namespace futures {
#ifndef FUTURES_DOXYGEN
    template <class R, class Options>
    class basic_future;
#endif
    namespace detail {
        // Traits we need for continuations
        // These are the most important traits we use in public functions
        // All other intermediary traits are left to next_future_traits_helper
        template <class Executor, class Function, class Future>
        struct next_future_traits {
            static constexpr bool is_valid_with_stop_token_only = mp_eval_if_c<
                continue_is_invocable_v<Future, Function>,
                std::false_type,
                continue_is_invocable,
                Future,
                Function,
                stop_token>::value;

            static constexpr bool is_valid
                = continue_is_invocable_v<Future, Function>
                  || is_valid_with_stop_token_only;

            static constexpr bool expects_stop_token
                = is_valid_with_stop_token_only;

            static constexpr bool should_inherit_stop_source
                = (has_stop_token_v<Future> && (!is_shared_future_v<Future>) )
                  && !expects_stop_token;

            using next_value_type = std::conditional_t<
                is_valid_with_stop_token_only,
                typename mp_eval_if_not<
                    conjunction<
                        is_stoppable<std::decay_t<Future>>,
                        std::is_same<
                            continue_invoke_result_t<Future, Function>,
                            continue_tags::failure>>,
                    mp_identity<continue_tags::failure>,
                    continue_invoke_result,
                    Future,
                    Function,
                    stop_token>::type,
                continue_invoke_result_t<Future, Function>>;

            using next_future_options = conditional_append_future_option_t<
                is_always_deferred_v<Future>,
                deferred_function_opt<
                    detail::future_continue_task<Future, Function>>,
                conditional_append_future_option_t<
                    is_always_deferred_v<Future>,
                    always_deferred_opt,
                    conditional_append_future_option_t<
                        is_valid_with_stop_token_only,
                        stoppable_opt,
                        std::conditional_t<
                            !is_always_deferred_v<Future>,
                            future_options<executor_opt<Executor>, continuable_opt>,
                            future_options<executor_opt<Executor>>>>>>;
        };

        template <class Executor, class Function, class Future>
        using next_options_t = typename next_future_traits<
            Executor,
            Function,
            Future>::next_future_options;

        template <class Executor, class Function, class Future>
        using next_value_t = typename next_future_traits<
            Executor,
            Function,
            Future>::next_value_type;

        template <class Executor, class Function, class Future>
        using next_future_t = basic_future<
            next_value_t<Executor, Function, Future>,
            next_options_t<Executor, Function, Future>>;

        template <class Executor, class Function, class Future>
        static constexpr bool next_future_is_valid_v
            = next_future_traits<Executor, Function, Future>::is_valid;

    } // namespace detail
} // namespace futures


#endif // FUTURES_ADAPTOR_DETAIL_NEXT_FUTURE_TRAITS_HPP
