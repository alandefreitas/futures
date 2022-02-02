//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_UNWRAP_AND_CONTINUE_HPP
#define FUTURES_UNWRAP_AND_CONTINUE_HPP

#include <futures/adaptor/detail/traits/range_or_tuple_value.hpp>

namespace futures::detail {
    struct unwrapping_failure_t
    {};

    // make it a functor to copy into deferred shared states
    struct unwrap_and_continue_functor
    {
        /// \brief Unwrap the results from `before` future object and give them
        /// to `continuation`
        ///
        /// This function unfortunately has very high cyclomatic complexity
        /// because it's the only way we can concatenate so many `if constexpr`
        /// without negating all previous conditions.
        ///
        /// \param before_future The antecedent future to be unwrapped
        /// \param continuation The continuation function
        /// \param args Arguments we send to the function before the unwrapped
        /// result (stop_token or <empty>) \return The continuation result
        template <
            class Future,
            typename Function,
            typename... PrefixArgs,
            std::enable_if_t<is_future_v<std::decay_t<Future>>, int> = 0>
        decltype(auto)
        operator()(
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            // Types we might use in continuation
            using value_type = future_value_t<Future>;
            using lvalue_type = std::add_lvalue_reference_t<value_type>;
            using rvalue_type = std::add_rvalue_reference_t<value_type>;

            // What kind of unwrapping is the continuation invocable with
            constexpr bool no_unwrap = std::
                is_invocable_v<Function, PrefixArgs..., Future>;
            constexpr bool no_input = std::
                is_invocable_v<Function, PrefixArgs...>;
            constexpr bool value_unwrap = std::
                is_invocable_v<Function, PrefixArgs..., value_type>;
            constexpr bool lvalue_unwrap = std::
                is_invocable_v<Function, PrefixArgs..., lvalue_type>;
            constexpr bool rvalue_unwrap = std::
                is_invocable_v<Function, PrefixArgs..., rvalue_type>;
            constexpr bool double_unwrap
                = is_future_v<std::decay_t<
                      value_type>> && std::is_invocable_v<Function, PrefixArgs..., future_value_t<value_type>>;
            constexpr bool is_tuple = is_tuple_v<value_type>;
            constexpr bool is_range = is_range_v<value_type>;

            // 5 main unwrapping paths: (no unwrap, no input, single future,
            // when_all, when_any)
            constexpr bool direct_unwrap = value_unwrap || lvalue_unwrap
                                           || rvalue_unwrap || double_unwrap;
            constexpr bool sequence_unwrap = is_tuple || is_range_v<value_type>;
            constexpr bool when_any_unwrap = is_when_any_result_v<value_type>;

            constexpr auto fail = []() {
                // Could not unwrap, return unwrapping_failure_t to indicate we
                // couldn't unwrap the continuation The function still needs to
                // be well-formed because other templates depend on it
                detail::throw_exception<std::logic_error>(
                    "Continuation unwrapping not possible");
                return unwrapping_failure_t{};
            };

            // Common continuations for basic_future
            if constexpr (no_unwrap) {
                return continuation(
                    std::forward<PrefixArgs>(prefix_args)...,
                    detail::move_or_copy(before_future));
            } else if constexpr (no_input) {
                before_future.get();
                return continuation(std::forward<PrefixArgs>(prefix_args)...);
            } else if constexpr (direct_unwrap) {
                value_type prev_state = before_future.get();
                if constexpr (value_unwrap) {
                    return continuation(
                        std::forward<PrefixArgs>(prefix_args)...,
                        std::move(prev_state));
                } else if constexpr (lvalue_unwrap) {
                    return continuation(
                        std::forward<PrefixArgs>(prefix_args)...,
                        prev_state);
                } else if constexpr (rvalue_unwrap) {
                    return continuation(
                        std::forward<PrefixArgs>(prefix_args)...,
                        std::move(prev_state));
                } else if constexpr (double_unwrap) {
                    return continuation(
                        std::forward<PrefixArgs>(prefix_args)...,
                        prev_state.get());
                } else {
                    return fail();
                }
            } else if constexpr (sequence_unwrap || when_any_unwrap) {
                using prefix_as_tuple = std::tuple<PrefixArgs...>;
                if constexpr (sequence_unwrap && is_tuple) {
                    constexpr bool tuple_explode = is_tuple_invocable_v<
                        Function,
                        tuple_type_concat_t<prefix_as_tuple, value_type>>;
                    constexpr bool is_future_tuple = tuple_type_all_of_v<
                        std::decay_t<value_type>,
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
                        using unwrapped_elements
                            = tuple_type_transform_t<value_type, future_value>;
                        constexpr bool tuple_explode_unwrap
                            = is_tuple_invocable_v<
                                Function,
                                tuple_type_concat_t<
                                    prefix_as_tuple,
                                    unwrapped_elements>>;
                        if constexpr (tuple_explode_unwrap) {
                            return transform_and_apply(
                                continuation,
                                [](auto &&el) {
                                if constexpr (!is_future_v<
                                                  std::decay_t<decltype(el)>>) {
                                    return el;
                                } else {
                                    return el.get();
                                }
                                },
                                std::tuple_cat(
                                    std::make_tuple(std::forward<PrefixArgs>(
                                        prefix_args)...),
                                    before_future.get()));
                        } else {
                            return fail();
                        }
                    } else {
                        return fail();
                    }
                } else if constexpr (sequence_unwrap && is_range) {
                    // when_all vector<future<T>> ->
                    // function(futures::small_vector<T>)
                    using range_value_t = range_value_t<value_type>;
                    constexpr bool is_range_of_futures = is_future_v<
                        std::decay_t<range_value_t>>;
                    using continuation_vector = detail::small_vector<
                        future_value_t<range_value_t>>;
                    using lvalue_continuation_vector = std::
                        add_lvalue_reference_t<continuation_vector>;
                    constexpr bool vector_unwrap
                        = is_range_of_futures
                          && (std::is_invocable_v<
                                  Function,
                                  PrefixArgs...,
                                  continuation_vector> || std::is_invocable_v<Function, PrefixArgs..., lvalue_continuation_vector>);
                    if constexpr (vector_unwrap) {
                        value_type futures_vector = before_future.get();
                        using future_vector_value_type = typename value_type::
                            value_type;
                        using unwrap_vector_value_type = future_value_t<
                            future_vector_value_type>;
                        using unwrap_vector_type = detail::small_vector<
                            unwrap_vector_value_type>;
                        unwrap_vector_type continuation_values;
                        std::transform(
                            futures_vector.begin(),
                            futures_vector.end(),
                            std::back_inserter(continuation_values),
                            [](future_vector_value_type &f) {
                            return f.get();
                            });
                        return continuation(
                            std::forward<PrefixArgs>(prefix_args)...,
                            continuation_values);
                    } else {
                        return fail();
                    }
                } else if constexpr (when_any_unwrap) {
                    // Common continuations for when_any futures
                    // when_any<tuple<future<T1>, future<T2>, ...>> ->
                    // function(size_t, tuple<future<T1>, future<T2>, ...>)
                    using when_any_index = typename value_type::size_type;
                    using when_any_sequence = typename value_type::sequence_type;
                    using when_any_members_as_tuple = std::
                        tuple<when_any_index, when_any_sequence>;
                    constexpr bool when_any_split = is_tuple_invocable_v<
                        Function,
                        tuple_type_concat_t<
                            prefix_as_tuple,
                            when_any_members_as_tuple>>;

                    // when_any<tuple<future<>,...>> -> function(size_t,
                    // future<T1>, future<T2>, ...)
                    constexpr bool when_any_explode = []() {
                        if constexpr (is_tuple_v<when_any_sequence>) {
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
                        = is_range_v<
                              when_any_sequence> || is_single_type_tuple_v<when_any_sequence>;
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
                                  std::tuple<
                                      future_value_t<when_any_element_type>>>>;

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
                    } else if constexpr (
                        when_any_element || when_any_unwrap_element) {
                        constexpr auto get_nth_future = [](auto &when_any_f) {
                            if constexpr (is_tuple_v<when_any_sequence>) {
                                return std::move(futures::get(
                                    std::move(when_any_f.tasks),
                                    when_any_f.index));
                            } else {
                                return std::move(
                                    when_any_f.tasks[when_any_f.index]);
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
                            return fail();
                        }
                    } else {
                        return fail();
                    }
                } else {
                    return fail();
                }
            } else {
                return fail();
            }
        }
    };

    constexpr unwrap_and_continue_functor unwrap_and_continue;

    /// \brief Find the result of unwrap and continue or return
    /// unwrapping_failure_t if expression is not well-formed
    template <class Future, class Function, class = void>
    struct result_of_unwrap
    {
        using type = unwrapping_failure_t;
    };

    template <class Future, class Function>
    struct result_of_unwrap<
        Future,
        Function,
        std::void_t<decltype(unwrap_and_continue_functor{}(
            std::declval<Future>(),
            std::declval<Function>()))>>
    {
        using type = decltype(unwrap_and_continue_functor{}(
            std::declval<Future>(),
            std::declval<Function>()));
    };

    template <class Future, class Function>
    using result_of_unwrap_t = typename result_of_unwrap<Future, Function>::type;

    /// \brief Find the result of unwrap and continue with token or return
    /// unwrapping_failure_t otherwise
    template <class Future, class Function, class = void>
    struct result_of_unwrap_with_token
    {
        using type = unwrapping_failure_t;
    };

    template <class Future, class Function>
    struct result_of_unwrap_with_token<
        Future,
        Function,
        std::void_t<decltype(unwrap_and_continue_functor{}(
            std::declval<Future>(),
            std::declval<Function>(),
            std::declval<stop_token>()))>>
    {
        using type = decltype(unwrap_and_continue_functor{}(
            std::declval<Future>(),
            std::declval<Function>(),
            std::declval<stop_token>()));
    };

    template <class Future, class Function>
    using result_of_unwrap_with_token_t =
        typename result_of_unwrap_with_token<Future, Function>::type;

    template <typename Function, typename Future>
    struct continuation_traits_helper
    {
        // The return type of unwrap and continue function
        using unwrap_result_no_token_type = result_of_unwrap_t<Future, Function>;
        using unwrap_result_with_token_type
            = result_of_unwrap_with_token_t<Future, Function>;

        // Whether the continuation expects a token
        static constexpr bool is_valid_without_stop_token = !std::is_same_v<
            unwrap_result_no_token_type,
            unwrapping_failure_t>;

        static constexpr bool is_valid_with_stop_token = !std::is_same_v<
            unwrap_result_with_token_type,
            unwrapping_failure_t>;

        // Whether the continuation is valid
        static constexpr bool is_valid = is_valid_without_stop_token
                                         || is_valid_with_stop_token;

        // The result type of unwrap and continue for the valid version, with or
        // without token
        using value_type = std::conditional_t<
            is_valid_with_stop_token,
            unwrap_result_with_token_type,
            unwrap_result_no_token_type>;

        // Stop token for the continuation function
        constexpr static bool continuation_expects_stop_token
            = is_valid_with_stop_token;

        // Check if the stop token should be inherited from previous future
        constexpr static bool previous_future_has_stop_token = has_stop_token_v<
            Future>;
        constexpr static bool previous_future_is_shared = is_shared_future_v<
            Future>;
        constexpr static bool inherit_stop_token
            = previous_future_has_stop_token && (!previous_future_is_shared);

        // Continuation future should have stop token
        constexpr static bool after_has_stop_token = is_valid_with_stop_token
                                                     || inherit_stop_token;

        // The result type of unwrap and continue for the valid version, with or
        // without token
        using eager_future_options = std::conditional_t<
            after_has_stop_token,
            future_options<continuable_opt, stoppable_opt>,
            future_options<continuable_opt>>;

        // The result type of unwrap and continue for the valid version, with or
        // without token
        using eager_future_type = basic_future<value_type, eager_future_options>;

        // The result type of unwrap and continue for the valid version, with or
        // without token
        using deferred_future_options = std::conditional_t
                                        < after_has_stop_token,
              future_options<continuable_opt, stoppable_opt, deferred_opt>,
              future_options<continuable_opt, deferred_opt>>;

        // The result type of unwrap and continue for the valid version, with or
        // without token
        using deferred_future_type
            = basic_future<value_type, deferred_future_options>;
    };

    template <typename Function, typename Future>
    struct continuation_traits
    {
        using helper = continuation_traits_helper<Function, Future>;

        static constexpr bool continuation_expects_stop_token = helper::
            continuation_expects_stop_token;

        static constexpr bool should_inherit_stop_source
            = helper::inherit_stop_token
              && !helper::continuation_expects_stop_token;

        using value_type =
            typename continuation_traits_helper<Function, Future>::value_type;

        using future_options = std::conditional_t<
            is_deferred_v<Future>,
            typename continuation_traits_helper<Function, Future>::
                deferred_future_options,
            typename continuation_traits_helper<Function, Future>::
                eager_future_options>;

        using future_type = basic_future<value_type, future_options>;
    };

} // namespace futures::detail

#endif // FUTURES_UNWRAP_AND_CONTINUE_HPP
