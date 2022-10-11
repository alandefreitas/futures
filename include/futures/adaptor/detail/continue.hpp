//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_CONTINUE_HPP
#define FUTURES_ADAPTOR_DETAIL_CONTINUE_HPP

#include <futures/config.hpp>
#include <futures/future_options.hpp>
#include <futures/stop_token.hpp>
#include <futures/traits/future_value.hpp>
#include <futures/traits/has_stop_token.hpp>
#include <futures/traits/is_always_deferred.hpp>
#include <futures/traits/is_shared_future.hpp>
#include <futures/traits/is_stoppable.hpp>
#include <futures/adaptor/detail/continue_invoke_tag.hpp>
#include <futures/detail/deps/boost/mp11/tuple.hpp>

namespace futures::detail {
    // --------------------------------------------------------------
    // continuation function
    //

    struct future_continue_functor : private continue_tags {
        // Unwrap the results from `before` future object and give them
        // to `continuation`
        /*
         *  before_future: The antecedent future to be unwrapped
         *  continuation: The continuation function
         *  args: Arguments we send to the function before the unwrapped
         *  result (stop_token or <empty>)
         */
        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_t<Future, Function, PrefixArgs...>
        operator()(
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            return operator()(
                continue_tag_t<Future, Function, PrefixArgs...>{},
                std::forward<Future>(before_future),
                std::forward<Function>(continuation),
                std::forward<PrefixArgs>(prefix_args)...);
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<no_unwrap, Future, Function, PrefixArgs...>
        operator()(
            no_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(before_future));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<no_input, Future, Function, PrefixArgs...>
        operator()(
            no_input,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            before_future.get();
            return continuation(std::forward<PrefixArgs>(prefix_args)...);
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            rvalue_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            rvalue_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            future_value_t<Future> prev_state = before_future.get();
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(prev_state));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            double_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            double_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                before_future.get().get());
        }

        template <class Future>
        static constexpr std::
            enable_if_t<is_future_v<Future>, unwrap_future_t<Future>>
            get_deepest(Future &&f) {
            return get_deepest(std::forward<Future>(f).get());
        }

        template <class Future>
        static constexpr std::
            enable_if_t<!is_future_v<Future>, unwrap_future_t<Future>>
            get_deepest(Future &&f) {
            return std::forward<Future>(f);
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            deepest_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            deepest_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                get_deepest(before_future));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            tuple_explode_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            tuple_explode_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            // future<tuple<future<T1>, future<T2>, ...>> ->
            // function(future<T1>, future<T2>, ...)
            return std::apply(
                continuation,
                std::tuple_cat(
                    std::make_tuple(std::forward<PrefixArgs>(prefix_args)...),
                    before_future.get()));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            futures_tuple_double_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            futures_tuple_double_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            auto future_to_value = [](auto &&el) {
                if constexpr (!is_future_v<std::decay_t<decltype(el)>>) {
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
                    std::tuple_cat(prefix_as_tuple, std::move(futures_tuple))));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            futures_tuple_deepest_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            futures_tuple_deepest_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            auto prefix_as_tuple = std::make_tuple(
                std::forward<PrefixArgs>(prefix_args)...);
            auto futures_tuple = before_future.get();
            // transform each tuple with future_to_value
            return std::apply(
                continuation,
                tuple_transform(
                    [](auto &&f) { return deepest_unwrap(f); },
                    std::tuple_cat(prefix_as_tuple, std::move(futures_tuple))));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            futures_range_double_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            futures_range_double_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            // when_all vector<future<T>> ->
            // function(futures::small_vector<T>)
            future_value_t<Future> futures_vector = before_future.get();
            using future_vector_value_type = typename future_value_t<
                Future>::value_type;
            detail::small_vector<future_value_t<future_vector_value_type>>
                continuation_values;
            continuation_values.reserve(futures_vector.size());
            for (auto &f: futures_vector)
                continuation_values.push_back(f.get());
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(continuation_values));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            futures_range_deepest_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            futures_range_deepest_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            // when_all vector<future<T>> ->
            // function(futures::small_vector<T>)
            future_value_t<Future> futures_vector = before_future.get();
            using future_vector_value_type = unwrap_future_t<Future>;
            detail::small_vector<unwrap_future_t<future_vector_value_type>>
                continuation_values;
            continuation_values.reserve(futures_vector.size());
            for (auto &f: futures_vector)
                continuation_values.push_back(get_deepest(f));
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(continuation_values));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            when_any_split_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            when_any_split_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            auto w = before_future.get();
            return std::apply(
                std::forward<Function>(continuation),
                std::make_tuple(
                    std::forward<PrefixArgs>(prefix_args)...,
                    w.index,
                    std::move(w.tasks)));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            when_any_explode_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            when_any_explode_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            auto w = before_future.get();
            return std::apply(
                std::forward<Function>(continuation),
                std::tuple_cat(
                    std::make_tuple(
                        std::forward<PrefixArgs>(prefix_args)...,
                        w.index),
                    std::move(w.tasks)));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            when_any_tuple_element_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            when_any_tuple_element_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            auto w = before_future.get();
            constexpr std::size_t N = std::tuple_size<
                std::decay_t<decltype(w.tasks)>>::value;
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(
                    // get w.index-th element of
                    // w.tasks
                    mp_with_index<N>(w.index, [&](auto I) {
                        // I is mp_size_t<v.index()>{} here
                        return std::move(std::get<I>(w.tasks));
                    })));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            when_any_range_element_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            when_any_range_element_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            auto w = before_future.get();
            auto nth_future = std::move(w.tasks[w.index]);
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(nth_future));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            when_any_tuple_double_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            when_any_tuple_double_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            auto w = before_future.get();
            constexpr std::size_t N = std::tuple_size<
                std::decay_t<decltype(w.tasks)>>::value;
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(
                    // get w.index-th element of
                    // w.tasks
                    mp_with_index<N>(w.index, [&](auto I) {
                        // I is mp_size_t<v.index()>{} here
                        return std::move(std::get<I>(w.tasks).get());
                    })));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            when_any_tuple_deepest_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            when_any_tuple_deepest_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            auto w = before_future.get();
            constexpr std::size_t N = std::tuple_size<
                std::decay_t<decltype(w.tasks)>>::value;
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(
                    // get w.index-th element of
                    // w.tasks
                    mp_with_index<N>(w.index, [&](auto I) {
                        // I is mp_size_t<v.index()>{} here
                        return std::move(get_deepest(std::get<I>(w.tasks)));
                    })));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            when_any_range_double_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            when_any_range_double_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            auto w = before_future.get();
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(w.tasks[w.index].get()));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_invoke_result_for_t<
            when_any_range_deepest_unwrap,
            Future,
            Function,
            PrefixArgs...>
        operator()(
            when_any_range_deepest_unwrap,
            Future &&before_future,
            Function &&continuation,
            PrefixArgs &&...prefix_args) const {
            auto w = before_future.get();
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                std::move(get_deepest(w.tasks[w.index])));
        }

        template <class Future, class Function, class... PrefixArgs>
        continue_tags::failure
        operator()(failure, Future &&, Function &&, PrefixArgs &&...) const {
            return continue_tags::failure{};
        }
    };

    constexpr future_continue_functor future_continue;

    // A functor that stores both future and the function for the continuation
    template <class Future, class Function>
    struct future_continue_task {
        Future before_;
        Function after_;

        decltype(auto)
        operator()() {
            return future_continue(std::move(before_), std::move(after_));
        }

        decltype(auto)
        operator()(stop_token st) {
            return future_continue(std::move(before_), std::move(after_), st);
        }
    };

    // Identify future_continue_task in case the shared state need to knows
    // how to handle it
    template <class Function>
    struct is_future_continue_task : std::false_type {};

    template <class Future, class Function>
    struct is_future_continue_task<future_continue_task<Future, Function>>
        : std::true_type {};

    // Find the result of unwrap and continue or return
    // continue_tags::failure if expression is not well-formed
    template <class Future, class Function, class = void>
    struct result_of_unwrap {
        using type = continue_tags::failure;
    };

    // When continuation is callable without token
    template <class Future, class Function>
    struct result_of_unwrap<
        Future,
        Function,
        std::void_t<decltype(future_continue_functor{}(
            std::declval<Future>(),
            std::declval<Function>()))>> {
        using type = decltype(future_continue_functor{}(
            std::declval<Future>(),
            std::declval<Function>()));
    };

    template <class Future, class Function>
    using result_of_unwrap_t = typename result_of_unwrap<Future, Function>::type;

    // Find the result of unwrap and continue with token or return
    // continue_tags::failure otherwise. The implementation avoids even
    // evaluating it if the previous future has no stop token
    template <bool Enable, class Future, class Function, class = void>
    struct result_of_unwrap_with_token_impl {
        using type = continue_tags::failure;
    };

    template <class Future, class Function>
    struct result_of_unwrap_with_token_impl<
        true,
        Future,
        Function,
        std::void_t<
            // unwrapping with stop token is possible
            decltype(future_continue_functor{}(
                std::declval<Future>(),
                std::declval<Function>(),
                std::declval<stop_token>()))>> {
        using type = decltype(future_continue_functor{}(
            std::declval<Future>(),
            std::declval<Function>(),
            std::declval<stop_token>()));
    };

    /// Find the result of unwrap and continue with token or return
    /// continue_tags::failure otherwise
    template <class Future, class Function>
    struct result_of_unwrap_with_token {
        using type = typename result_of_unwrap_with_token_impl<
            // clang-format off
            // only attempt to invoke the function if:
            // previous future is stoppable
            is_stoppable_v<std::decay_t<Future>> &&
            // unwrapping without stop token fails
            std::is_same_v<result_of_unwrap_t<Future, Function>, continue_tags::failure>
            // clang-format on
            ,
            Future,
            Function>::type;
    };

    template <class Future, class Function>
    using result_of_unwrap_with_token_t =
        typename result_of_unwrap_with_token<Future, Function>::type;

    // Intermediary traits we need for continuations
    // According to what kind of future comes before and the function type
    // we might need the continuation future to include a stop token.
    template <class Executor, class Function, class Future>
    struct continuation_traits_helper {
        // The possible return types of unwrap and continue function
        using unwrap_result = result_of_unwrap_t<Future, Function>;
        using unwrap_result_with_token_prefix
            = result_of_unwrap_with_token_t<Future, Function>;

        // Whether the continuation expects a token
        static constexpr bool is_valid_without_stop_token
            = !std::is_same_v<unwrap_result, continue_tags::failure>;
        static constexpr bool is_valid_with_stop_token = !std::is_same_v<
            unwrap_result_with_token_prefix,
            continue_tags::failure>;

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
                    detail::future_continue_task<Future, Function>>,
                next_maybe_deferred_future_options>;

        // The result options type of unwrap and continue
        using next_future_options = next_maybe_function_type_future_options;
    };

    // Traits we need for continuations
    // These are the most important traits we use in public functions
    // All other intermediary traits are left to continuation_traits_helper
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

#endif // FUTURES_ADAPTOR_DETAIL_CONTINUE_HPP
