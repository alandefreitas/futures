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

namespace futures {
    namespace detail {
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
            continue_invoke_result_for_t<
                no_unwrap,
                Future,
                Function,
                PrefixArgs...>
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
                return tuple_apply(
                    continuation,
                    std::tuple_cat(
                        std::make_tuple(
                            std::forward<PrefixArgs>(prefix_args)...),
                        before_future.get()));
            }

            struct future_to_value {
                template <class T>
                auto
                operator()(T &&el) const {
                    return impl(
                        is_future<std::decay_t<T>>{},
                        std::forward<T>(el));
                }

            private:
                template <class T>
                auto
                impl(std::true_type, T &&f) const {
                    return f.get();
                }

                template <class T>
                auto
                impl(std::false_type, T &&v) const {
                    return v;
                }
            };

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
                auto prefix_as_tuple = std::make_tuple(
                    std::forward<PrefixArgs>(prefix_args)...);
                auto futures_tuple = before_future.get();
                // transform each tuple with future_to_value
                return tuple_apply(
                    continuation,
                    tuple_transform(
                        future_to_value{},
                        std::tuple_cat(
                            prefix_as_tuple,
                            std::move(futures_tuple))));
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
                return tuple_apply(
                    continuation,
                    tuple_transform(
                        [](auto &&f) { return deepest_unwrap(f); },
                        std::tuple_cat(
                            prefix_as_tuple,
                            std::move(futures_tuple))));
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
                for (auto &f: futures_vector) {
                    continuation_values.push_back(f.get());
                }
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
                for (auto &f: futures_vector) {
                    continuation_values.push_back(get_deepest(f));
                }
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
                return tuple_apply(
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
                return tuple_apply(
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
            continue_invoke_result_for_t<failure, Future, Function, PrefixArgs...>
            operator()(failure, Future &&, Function &&, PrefixArgs &&...)
                const {
                return continue_tags::failure{};
            }
        };
    } // namespace detail
} // namespace futures

#endif // FUTURES_ADAPTOR_DETAIL_CONTINUE_HPP
