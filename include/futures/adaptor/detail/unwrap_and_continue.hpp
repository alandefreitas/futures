//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_UNWRAP_AND_CONTINUE_HPP
#define FUTURES_ADAPTOR_DETAIL_UNWRAP_AND_CONTINUE_HPP

#include <futures/config.hpp>
#include <futures/future_options.hpp>
#include <futures/adaptor/detail/unwrap_and_continue_traits.hpp>
#include <futures/detail/container/small_vector.hpp>
#include <futures/detail/exception/throw_exception.hpp>
#include <futures/detail/move_if_not_shared.hpp>
#include <futures/detail/traits/append_future_option.hpp>
#include <futures/detail/traits/is_single_type_tuple.hpp>
#include <futures/detail/traits/is_tuple_invocable.hpp>
#include <futures/detail/traits/is_when_any_result.hpp>
#include <futures/detail/traits/range_or_tuple_value.hpp>
#include <futures/detail/traits/tuple_type_all_of.hpp>
#include <futures/detail/traits/tuple_type_concat.hpp>
#include <futures/detail/traits/tuple_type_transform.hpp>
#include <futures/detail/deps/boost/mp11/algorithm.hpp>
#include <futures/detail/deps/boost/mp11/tuple.hpp>

namespace futures::detail {
    struct unwrapping_failure_t {};

    struct unwrap_and_continue_functor {
        /// Unwrap the results from `before` future object and give them
        /// to `continuation`
        ///
        /// This function unfortunately has very high cyclomatic complexity
        /// because it's the only way we can concatenate so many `if constexpr`
        /// without negating all previous conditions.
        ///
        /// @param before_future The antecedent future to be unwrapped
        /// @param continuation The continuation function
        /// @param args Arguments we send to the function before the unwrapped
        /// result (stop_token or <empty>)
        ///
        /// @return The continuation result
        template <
            class Future,
            typename Function,
            typename... PrefixArgs,
            std::enable_if_t<
                // clang-format off
                is_no_unwrap_continuation_v<Future, Function, PrefixArgs...>
                // clang-format on
                ,
                int>
            = 0>
        decltype(auto)
        operator()(
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                detail::move_if_not_shared(before_future));
        }

        template <
            class Future,
            typename Function,
            typename... PrefixArgs,
            std::enable_if_t<
                // clang-format off
                is_no_input_continuation_v<Future, Function, PrefixArgs...>
                // clang-format on
                ,
                int>
            = 0>
        decltype(auto)
        operator()(
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            before_future.get();
            return continuation(std::forward<PrefixArgs>(prefix_args)...);
        }

        template <
            class Future,
            typename Function,
            typename... PrefixArgs,
            std::enable_if_t<
                // clang-format off
                is_value_unwrap_continuation_v<Future, Function, PrefixArgs...>
                // clang-format on
                ,
                int>
            = 0>
        decltype(auto)
        operator()(
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            future_value_t<Future> prev_state = before_future.get();
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(prev_state));
        }

        template <
            class Future,
            typename Function,
            typename... PrefixArgs,
            std::enable_if_t<
                // clang-format off
                is_lvalue_unwrap_continuation_v<Future, Function, PrefixArgs...>
                // clang-format on
                ,
                int>
            = 0>
        decltype(auto)
        operator()(
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            future_value_t<Future> prev_state = before_future.get();
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                prev_state);
        }

        template <
            class Future,
            typename Function,
            typename... PrefixArgs,
            std::enable_if_t<
                // clang-format off
                is_rvalue_unwrap_continuation_v<Future, Function, PrefixArgs...>
                // clang-format on
                ,
                int>
            = 0>
        decltype(auto)
        operator()(
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            future_value_t<Future> prev_state = before_future.get();
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(prev_state));
        }

        template <
            class Future,
            typename Function,
            typename... PrefixArgs,
            std::enable_if_t<
                // clang-format off
                is_double_unwrap_continuation_v<Future, Function, PrefixArgs...>
                // clang-format on
                ,
                int>
            = 0>
        decltype(auto)
        operator()(
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                before_future.get().get());
        }

        template <
            class Future,
            typename Function,
            typename... PrefixArgs,
            std::enable_if_t<
                // clang-format off
                is_tuple_unwrap_continuation_v<Future, Function, PrefixArgs...>
                // clang-format on
                ,
                int>
            = 0>
        decltype(auto)
        operator()(
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            constexpr bool tuple_explode = is_tuple_invocable_v<
                Function,
                tuple_type_concat_t<
                    std::tuple<PrefixArgs...>,
                    future_value_t<Future>>>;
            constexpr bool is_future_tuple = tuple_type_all_of_v<
                std::decay_t<future_value_t<Future>>,
                is_future>;
            if constexpr (tuple_explode) {
                // future<tuple<future<T1>, future<T2>, ...>> ->
                // function(future<T1>, future<T2>, ...)
                return std::apply(
                    continuation,
                    std::tuple_cat(
                        std::make_tuple(
                            std::forward<PrefixArgs>(prefix_args)...),
                        before_future.get()));
            } else if constexpr (is_future_tuple) {
                // future<tuple<future<T1>, future<T2>, ...>> ->
                // function(T1, T2, ...)
                using unwrapped_elements = tuple_type_transform_t<
                    future_value_t<Future>,
                    future_value>;
                constexpr bool tuple_explode_unwrap = is_tuple_invocable_v<
                    Function,
                    tuple_type_concat_t<
                        std::tuple<PrefixArgs...>,
                        unwrapped_elements>>;
                if constexpr (tuple_explode_unwrap) {
                    auto future_to_value = [](auto &&el) {
                        if constexpr (!is_future_v<std::decay_t<decltype(el)>>)
                        {
                            return el;
                        } else {
                            return el.get();
                        }
                    };
                    auto prefix_as_tuple = std::make_tuple(
                        std::forward<PrefixArgs>(prefix_args)...);
                    auto futures_tuple = before_future.get();
                    // transform each tuple with future_to_value
                    return std::apply(
                        continuation,
                        tuple_transform(
                            future_to_value,
                            std::tuple_cat(
                                prefix_as_tuple,
                                std::move(futures_tuple))));
                } else {
                    detail::throw_exception<std::logic_error>(
                        "Continuation unwrapping not possible");
                    return unwrapping_failure_t{};
                }
            } else {
                detail::throw_exception<std::logic_error>(
                    "Continuation unwrapping not possible");
                return unwrapping_failure_t{};
            }
        }

        template <
            class Future,
            typename Function,
            typename... PrefixArgs,
            std::enable_if_t<
                // clang-format off
                is_range_unwrap_continuation_v<Future, Function, PrefixArgs...>
                // clang-format on
                ,
                int>
            = 0>
        decltype(auto)
        operator()(
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            // when_all vector<future<T>> ->
            // function(futures::small_vector<T>)
            using range_value_t = range_value_t<future_value_t<Future>>;
            constexpr bool is_range_of_futures = is_future_v<
                std::decay_t<range_value_t>>;
            if constexpr (is_range_of_futures) {
                using continuation_vector = detail::small_vector<
                    future_value_t<range_value_t>>;
                using lvalue_continuation_vector = std::add_lvalue_reference_t<
                    continuation_vector>;
                constexpr bool vector_unwrap
                    = is_range_of_futures
                      && (std::is_invocable_v<
                              Function,
                              PrefixArgs...,
                              continuation_vector>
                          || std::is_invocable_v<
                              Function,
                              PrefixArgs...,
                              lvalue_continuation_vector>);
                if constexpr (vector_unwrap) {
                    future_value_t<Future> futures_vector = before_future.get();
                    using future_vector_value_type = typename future_value_t<
                        Future>::value_type;
                    using unwrap_vector_value_type = future_value_t<
                        future_vector_value_type>;
                    using unwrap_vector_type = detail::small_vector<
                        unwrap_vector_value_type>;
                    unwrap_vector_type continuation_values;
                    std::transform(
                        futures_vector.begin(),
                        futures_vector.end(),
                        std::back_inserter(continuation_values),
                        [](future_vector_value_type &f) { return f.get(); });
                    return continuation(
                        std::forward<PrefixArgs>(prefix_args)...,
                        continuation_values);
                } else {
                    detail::throw_exception<std::logic_error>(
                        "Continuation unwrapping not possible");
                    return unwrapping_failure_t{};
                }
            } else {
                detail::throw_exception<std::logic_error>(
                    "Continuation unwrapping not possible");
                return unwrapping_failure_t{};
            }
        }

        template <
            class Future,
            typename Function,
            typename... PrefixArgs,
            std::enable_if_t<
                // clang-format off
                is_when_any_unwrap_continuation_v<Future, Function, PrefixArgs...>
                // clang-format on
                ,
                int>
            = 0>
        decltype(auto)
        operator()(
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            using prefix_as_tuple = std::tuple<PrefixArgs...>;
            // Common continuations for when_any futures
            // when_any<tuple<future<T1>, future<T2>, ...>> ->
            // function(size_t, tuple<future<T1>, future<T2>, ...>)
            using when_any_index = typename future_value_t<Future>::size_type;
            using when_any_sequence = typename future_value_t<
                Future>::sequence_type;
            using when_any_members_as_tuple = std::
                tuple<when_any_index, when_any_sequence>;
            constexpr bool when_any_split = is_tuple_invocable_v<
                Function,
                tuple_type_concat_t<prefix_as_tuple, when_any_members_as_tuple>>;

            // when_any<tuple<future<>,...>> -> function(size_t,
            // future<T1>, future<T2>, ...)
            constexpr bool when_any_explode = []() {
                if constexpr (detail::is_tuple_v<when_any_sequence>) {
                    return is_tuple_invocable_v<
                        Function,
                        tuple_type_concat_t<
                            prefix_as_tuple,
                            std::tuple<when_any_index>,
                            when_any_sequence>>;
                } else {
                    return false;
                }
            }();

            // when_any_result<tuple<future<T>, future<T>, ...>> ->
            // continuation(future<T>)
            constexpr bool when_any_same_type
                = is_range_v<when_any_sequence>
                  || is_single_type_tuple_v<when_any_sequence>;
            using when_any_element_type = range_or_tuple_value_t<
                when_any_sequence>;
            constexpr bool when_any_element
                = when_any_same_type
                  && is_tuple_invocable_v<
                      Function,
                      tuple_type_concat_t<
                          prefix_as_tuple,
                          std::tuple<when_any_element_type>>>;

            // when_any_result<tuple<future<T>, future<T>, ...>> ->
            // continuation(T)
            constexpr bool when_any_unwrap_element
                = when_any_same_type
                  && is_tuple_invocable_v<
                      Function,
                      tuple_type_concat_t<
                          prefix_as_tuple,
                          std::tuple<future_value_t<when_any_element_type>>>>;

            auto w = before_future.get();
            if constexpr (when_any_split) {
                return std::apply(
                    continuation,
                    std::make_tuple(
                        std::forward<PrefixArgs>(prefix_args)...,
                        w.index,
                        std::move(w.tasks)));
            } else if constexpr (when_any_explode) {
                return std::apply(
                    continuation,
                    std::tuple_cat(
                        std::make_tuple(
                            std::forward<PrefixArgs>(prefix_args)...,
                            w.index),
                        std::move(w.tasks)));
            } else if constexpr (when_any_element || when_any_unwrap_element) {
                constexpr auto get_nth_future = [](auto &when_any_f) {
                    if constexpr (detail::is_tuple_v<when_any_sequence>) {
                        constexpr std::size_t N = std::tuple_size<
                            std::decay_t<decltype(when_any_f.tasks)>>::value;
                        return std::move(
                            // get when_any_f.index-th element of
                            // when_any_f.tasks
                            mp_with_index<N>(when_any_f.index, [&](auto I) {
                                // I is mp_size_t<v.index()>{} here
                                return std::move(std::get<I>(when_any_f.tasks));
                            }));
                    } else {
                        return std::move(when_any_f.tasks[when_any_f.index]);
                    }
                };
                auto nth_future = get_nth_future(w);
                if constexpr (when_any_element) {
                    return continuation(
                        std::forward<PrefixArgs>(prefix_args)...,
                        std::move(nth_future));
                } else if constexpr (when_any_unwrap_element) {
                    return continuation(
                        std::forward<PrefixArgs>(prefix_args)...,
                        std::move(nth_future.get()));
                } else {
                    detail::throw_exception<std::logic_error>(
                        "Continuation unwrapping not possible");
                    return unwrapping_failure_t{};
                }
            } else {
                detail::throw_exception<std::logic_error>(
                    "Continuation unwrapping not possible");
                return unwrapping_failure_t{};
            }
        }

        template <
            class Future,
            typename Function,
            typename... PrefixArgs,
            std::enable_if_t<
                // clang-format off
                !is_valid_unwrap_continuation_v<Future, Function, PrefixArgs...>
                // clang-format on
                ,
                int>
            = 0>
        decltype(auto)
        operator()(Future &&, Function &&, PrefixArgs &&...) const {
            // Could not unwrap, return unwrapping_failure_t to indicate we
            // couldn't unwrap the continuation. The function still needs to
            // be well-formed to facilitate other templates that depend on it
            detail::throw_exception<std::logic_error>(
                "Continuation unwrapping not possible");
            return unwrapping_failure_t{};
        }
    };

    constexpr unwrap_and_continue_functor unwrap_and_continue;


    template <class Future, class Function>
    struct unwrap_and_continue_task {
        Future before_;
        Function after_;

        decltype(auto)
        operator()() {
            return unwrap_and_continue(std::move(before_), std::move(after_));
        }

        decltype(auto)
        operator()(stop_token st) {
            return unwrap_and_continue(
                std::move(before_),
                std::move(after_),
                st);
        }
    };

    template <class Function>
    struct is_unwrap_and_continue_task : std::false_type {};

    template <class Future, class Function>
    struct is_unwrap_and_continue_task<
        unwrap_and_continue_task<Future, Function>> : std::true_type {};

    /// Find the result of unwrap and continue or return
    /// unwrapping_failure_t if expression is not well-formed
    template <class Future, class Function, class = void>
    struct result_of_unwrap {
        using type = unwrapping_failure_t;
    };

    template <class Future, class Function>
    struct result_of_unwrap<
        Future,
        Function,
        std::void_t<decltype(unwrap_and_continue_functor{}(
            std::declval<Future>(),
            std::declval<Function>()))>> {
        using type = decltype(unwrap_and_continue_functor{}(
            std::declval<Future>(),
            std::declval<Function>()));
    };

    template <class Future, class Function>
    using result_of_unwrap_t = typename result_of_unwrap<Future, Function>::type;

    /// Find the result of unwrap and continue with token or return
    /// unwrapping_failure_t otherwise. The implementation avoids even trying
    /// if the previous future has no stop token
    template <bool Enable, class Future, class Function, class = void>
    struct result_of_unwrap_with_token_impl {
        using type = unwrapping_failure_t;
    };

    template <class Future, class Function>
    struct result_of_unwrap_with_token_impl<
        true,
        Future,
        Function,
        std::void_t<
            // unwrapping with stop token is possible
            decltype(unwrap_and_continue_functor{}(
                std::declval<Future>(),
                std::declval<Function>(),
                std::declval<stop_token>()))>> {
        using type = decltype(unwrap_and_continue_functor{}(
            std::declval<Future>(),
            std::declval<Function>(),
            std::declval<stop_token>()));
    };

    /// Find the result of unwrap and continue with token or return
    /// unwrapping_failure_t otherwise
    template <class Future, class Function>
    struct result_of_unwrap_with_token {
        using type = typename result_of_unwrap_with_token_impl<
            // clang-format off
            // only attempt to invoke the function if:
            // previous future is stoppable
            is_stoppable_v<std::decay_t<Future>> &&
            // unwrapping without stop token fails
            std::is_same_v<result_of_unwrap_t<Future, Function>,unwrapping_failure_t>
            // clang-format on
            ,
            Future,
            Function>::type;
    };

    template <class Future, class Function>
    using result_of_unwrap_with_token_t =
        typename result_of_unwrap_with_token<Future, Function>::type;

    template <class Executor, class Function, class Future>
    struct continuation_traits_helper {
        // The possible return types of unwrap and continue function
        using unwrap_result = result_of_unwrap_t<Future, Function>;
        using unwrap_result_with_token_prefix
            = result_of_unwrap_with_token_t<Future, Function>;

        // Whether the continuation expects a token
        static constexpr bool is_valid_without_stop_token
            = !std::is_same_v<unwrap_result, unwrapping_failure_t>;
        static constexpr bool is_valid_with_stop_token = !std::is_same_v<
            unwrap_result_with_token_prefix,
            unwrapping_failure_t>;

        // Whether the continuation is valid at all
        static constexpr bool is_valid = is_valid_without_stop_token
                                         || is_valid_with_stop_token;

        // The result type of unwrap and continue for the valid overload
        // (with or without the token)
        using next_value_type = std::conditional_t<
            is_valid_with_stop_token,
            unwrap_result_with_token_prefix,
            unwrap_result>;

        // Stop token for the continuation function
        static constexpr bool expects_stop_token = is_valid_with_stop_token;

        // Check if the stop token can be inherited from to next future
        static constexpr bool previous_future_has_stop_token = has_stop_token_v<
            Future>;
        static constexpr bool previous_future_is_shared = is_shared_future_v<
            Future>;
        static constexpr bool can_inherit_stop_token
            = previous_future_has_stop_token && (!previous_future_is_shared);

        // Continuation future should have stop token
        // note: this is separate from `expects_stop_token` because (in the
        // future), the continuation might reuse the stop source without
        // actually containing a function that expects the token.
        static constexpr bool after_has_stop_token = expects_stop_token;

        // The result type of unwrap and continue for the valid unwrap overload
        // (with or without token)

        // Next needs to inherit the constructor from previous future
        // Next needs continuation source if previous is eager
        using next_maybe_continuable_future_options = std::conditional_t<
            !is_always_deferred_v<Future>,
            future_options<executor_opt<Executor>, continuable_opt>,
            future_options<executor_opt<Executor>>>;

        // Next is stoppable if we identified the function expects a token
        using next_maybe_stoppable_future_options
            = conditional_append_future_option_t<
                after_has_stop_token,
                stoppable_opt,
                next_maybe_continuable_future_options>;

        // Next needs the always_deferred_opt if it's deferred
        using next_maybe_deferred_future_options
            = conditional_append_future_option_t<
                is_always_deferred_v<Future>,
                always_deferred_opt,
                next_maybe_stoppable_future_options>;

        // Next needs the continuation function type if it's deferred
        using next_maybe_function_type_future_options
            = conditional_append_future_option_t<
                is_always_deferred_v<Future>,
                deferred_function_opt<
                    detail::unwrap_and_continue_task<Future, Function>>,
                next_maybe_deferred_future_options>;

        // The result options type of unwrap and continue
        using next_future_options = next_maybe_function_type_future_options;
    };

    template <class Executor, class Function, class Future>
    struct continuation_traits {
        using helper = continuation_traits_helper<Executor, Function, Future>;

        static constexpr bool is_valid = helper::is_valid;

        static constexpr bool expects_stop_token = helper::expects_stop_token;

        static constexpr bool should_inherit_stop_source
            = helper::can_inherit_stop_token && !helper::expects_stop_token;

        using next_value_type = typename helper::next_value_type;

        using next_future_options = typename helper::next_future_options;
    };


} // namespace futures::detail

#endif // FUTURES_ADAPTOR_DETAIL_UNWRAP_AND_CONTINUE_HPP
