//
// Copyright (c) alandefreitas 12/5/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_CONTINUATION_UNWRAP_H
#define FUTURES_CONTINUATION_UNWRAP_H

#include <futures/adaptor/detail/traits/is_callable.h>
#include <futures/adaptor/detail/traits/is_single_type_tuple.h>
#include <futures/adaptor/detail/traits/is_tuple.h>
#include <futures/adaptor/detail/traits/is_tuple_invocable.h>
#include <futures/adaptor/detail/traits/is_when_any_result.h>
#include <futures/adaptor/detail/traits/tuple_type_all_of.h>
#include <futures/adaptor/detail/traits/tuple_type_concat.h>
#include <futures/adaptor/detail/traits/tuple_type_transform.h>
#include <futures/adaptor/detail/traits/type_member_or.h>
#include <futures/adaptor/detail/tuple_algorithm.h>
#include <futures/algorithm/detail/traits/range/range/concepts.h>
#include <futures/futures/detail/traits/type_member_or_void.h>

#include <futures/futures/basic_future.h>
#include <futures/futures/traits/is_future.h>
#include <futures/futures/traits/unwrap_future.h>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    struct unwrapping_failure_t {};

    /// \brief Get the element type of a when any result object
    /// This is a very specific helper trait we need
    template <typename T, class Enable = void> struct range_or_tuple_element_type {};

    template <typename Sequence> struct range_or_tuple_element_type<Sequence, std::enable_if_t<range<Sequence>>> {
        using type = range_value_t<Sequence>;
    };

    template <typename Sequence> struct range_or_tuple_element_type<Sequence, std::enable_if_t<is_tuple_v<Sequence>>> {
        using type = std::tuple_element_t<0, Sequence>;
    };

    template <class T> using range_or_tuple_element_type_t = typename range_or_tuple_element_type<T>::type;

    /// \brief Unwrap the results from `before` future object and give them to `continuation`
    ///
    /// This function unfortunately has very high cyclomatic complexity because it's the only way
    /// we can concatenate so many `if constexpr` without negating all previous conditions.
    ///
    /// \param before_future The antecedent future to be unwrapped
    /// \param continuation The continuation function
    /// \param args Arguments we send to the function before the unwrapped result (stop_token or <empty>)
    /// \return The continuation result
    template <class Future, typename Function, typename... PrefixArgs, std::enable_if_t<is_future_v<Future>, int> = 0>
    decltype(auto) unwrap_and_continue(Future &&before_future, Function &&continuation, PrefixArgs &&...prefix_args) {
        // Types we might use in continuation
        using value_type = unwrap_future_t<Future>;
        using lvalue_type = std::add_lvalue_reference_t<value_type>;
        using rvalue_type = std::add_rvalue_reference_t<value_type>;

        // What kind of unwrapping is the continuation invocable with
        constexpr bool no_unwrap = std::is_invocable_v<Function, PrefixArgs..., Future>;
        constexpr bool no_input = std::is_invocable_v<Function, PrefixArgs...>;
        constexpr bool value_unwrap = std::is_invocable_v<Function, PrefixArgs..., value_type>;
        constexpr bool lvalue_unwrap = std::is_invocable_v<Function, PrefixArgs..., lvalue_type>;
        constexpr bool rvalue_unwrap = std::is_invocable_v<Function, PrefixArgs..., rvalue_type>;
        constexpr bool double_unwrap =
            is_future_v<value_type> && std::is_invocable_v<Function, PrefixArgs..., unwrap_future_t<value_type>>;
        constexpr bool is_tuple = is_tuple_v<value_type>;
        constexpr bool is_range = range<value_type>;

        // 5 main unwrapping paths: (no unwrap, no input, single future, when_all, when_any)
        constexpr bool direct_unwrap = value_unwrap || lvalue_unwrap || rvalue_unwrap || double_unwrap;
        constexpr bool sequence_unwrap = is_tuple || range<value_type>;
        constexpr bool when_any_unwrap = is_when_any_result_v<value_type>;

        constexpr auto fail = []() {
            // Could not unwrap, return unwrapping_failure_t to indicate we couldn't unwrap the continuation
            // The function still needs to be well-formed because other templates depend on it
            detail::throw_exception<std::logic_error>("Continuation unwrapping not possible");
            return unwrapping_failure_t{};
        };

        // Common continuations for basic_future
        if constexpr (no_unwrap) {
            return continuation(std::forward<PrefixArgs>(prefix_args)..., move_or_copy(before_future));
        } else if constexpr (no_input) {
            before_future.get();
            return continuation(std::forward<PrefixArgs>(prefix_args)...);
        } else if constexpr (direct_unwrap) {
            value_type prev_state = before_future.get();
            if constexpr (value_unwrap) {
                return continuation(std::forward<PrefixArgs>(prefix_args)..., std::move(prev_state));
            } else if constexpr (lvalue_unwrap) {
                return continuation(std::forward<PrefixArgs>(prefix_args)..., prev_state);
            } else if constexpr (rvalue_unwrap) {
                return continuation(std::forward<PrefixArgs>(prefix_args)..., std::move(prev_state));
            } else if constexpr (double_unwrap) {
                return continuation(std::forward<PrefixArgs>(prefix_args)..., prev_state.get());
            } else {
                return fail();
            }
        } else if constexpr (sequence_unwrap || when_any_unwrap) {
            using prefix_as_tuple = std::tuple<PrefixArgs...>;
            if constexpr (sequence_unwrap && is_tuple) {
                constexpr bool tuple_explode =
                    is_tuple_invocable_v<Function, tuple_type_concat_t<prefix_as_tuple, value_type>>;
                constexpr bool is_future_tuple = tuple_type_all_of_v<value_type, is_future>;
                if constexpr (tuple_explode) {
                    // future<tuple<future<T1>, future<T2>, ...>> -> function(future<T1>, future<T2>, ...)
                    return std::apply(
                        continuation,
                        std::tuple_cat(std::make_tuple(std::forward<PrefixArgs>(prefix_args)...), before_future.get()));
                } else if constexpr (is_future_tuple) {
                    // future<tuple<future<T1>, future<T2>, ...>> -> function(T1, T2, ...)
                    using unwrapped_elements = tuple_type_transform_t<value_type, unwrap_future>;
                    constexpr bool tuple_explode_unwrap =
                        is_tuple_invocable_v<Function, tuple_type_concat_t<prefix_as_tuple, unwrapped_elements>>;
                    if constexpr (tuple_explode_unwrap) {
                        return transform_and_apply(
                            continuation,
                            [](auto &&el) {
                                if constexpr (!is_future_v<std::decay_t<decltype(el)>>) {
                                    return el;
                                } else {
                                    return el.get();
                                }
                            },
                            std::tuple_cat(std::make_tuple(std::forward<PrefixArgs>(prefix_args)...),
                                           before_future.get()));
                    } else {
                        return fail();
                    }
                } else {
                    return fail();
                }
            } else if constexpr (sequence_unwrap && is_range) {
                // when_all vector<future<T>> -> function(futures::small_vector<T>)
                using range_value_t = detail::range_value_t<value_type>;
                constexpr bool is_range_of_futures = is_future_v<range_value_t>;
                using continuation_vector = futures::small_vector<unwrap_future_t<range_value_t>>;
                using lvalue_continuation_vector = std::add_lvalue_reference_t<continuation_vector>;
                constexpr bool vector_unwrap =
                    is_range_of_futures && (std::is_invocable_v<Function, PrefixArgs..., continuation_vector> ||
                                            std::is_invocable_v<Function, PrefixArgs..., lvalue_continuation_vector>);
                if constexpr (vector_unwrap) {
                    value_type futures_vector = before_future.get();
                    using future_vector_value_type = typename value_type::value_type;
                    using unwrap_vector_value_type = unwrap_future_t<future_vector_value_type>;
                    using unwrap_vector_type = ::futures::small_vector<unwrap_vector_value_type>;
                    unwrap_vector_type continuation_values;
                    std::transform(futures_vector.begin(), futures_vector.end(),
                                   std::back_inserter(continuation_values),
                                   [](future_vector_value_type &f) { return f.get(); });
                    return continuation(std::forward<PrefixArgs>(prefix_args)..., continuation_values);
                } else {
                    return fail();
                }
            } else if constexpr (when_any_unwrap) {
                // Common continuations for when_any futures
                // when_any<tuple<future<T1>, future<T2>, ...>> -> function(size_t, tuple<future<T1>, future<T2>, ...>)
                using when_any_index = typename value_type::size_type;
                using when_any_sequence = typename value_type::sequence_type;
                using when_any_members_as_tuple = std::tuple<when_any_index, when_any_sequence>;
                constexpr bool when_any_split =
                    is_tuple_invocable_v<Function, tuple_type_concat_t<prefix_as_tuple, when_any_members_as_tuple>>;

                // when_any<tuple<future<>,...>> -> function(size_t, future<T1>, future<T2>, ...)
                constexpr bool when_any_explode = []() {
                    if constexpr (is_tuple_v<when_any_sequence>) {
                        return is_tuple_invocable_v<
                            Function,
                            tuple_type_concat_t<prefix_as_tuple, std::tuple<when_any_index>, when_any_sequence>>;
                    } else {
                        return false;
                    }
                }();

                // when_any_result<tuple<future<T>, future<T>, ...>> -> continuation(future<T>)
                constexpr bool when_any_same_type =
                    range<when_any_sequence> || is_single_type_tuple_v<when_any_sequence>;
                using when_any_element_type = range_or_tuple_element_type_t<when_any_sequence>;
                constexpr bool when_any_element =
                    when_any_same_type &&
                    is_tuple_invocable_v<Function,
                                         tuple_type_concat_t<prefix_as_tuple, std::tuple<when_any_element_type>>>;

                // when_any_result<tuple<future<T>, future<T>, ...>> -> continuation(T)
                constexpr bool when_any_unwrap_element =
                    when_any_same_type &&
                    is_tuple_invocable_v<
                        Function,
                        tuple_type_concat_t<prefix_as_tuple, std::tuple<unwrap_future_t<when_any_element_type>>>>;

                auto w = before_future.get();
                if constexpr (when_any_split) {
                    return std::apply(continuation, std::make_tuple(std::forward<PrefixArgs>(prefix_args)..., w.index,
                                                                    std::move(w.tasks)));
                } else if constexpr (when_any_explode) {
                    return std::apply(continuation,
                                      std::tuple_cat(std::make_tuple(std::forward<PrefixArgs>(prefix_args)..., w.index),
                                                     std::move(w.tasks)));
                } else if constexpr (when_any_element || when_any_unwrap_element) {
                    constexpr auto get_nth_future = [](auto &when_any_f) {
                        if constexpr (is_tuple_v<when_any_sequence>) {
                            return std::move(futures::get(std::move(when_any_f.tasks), when_any_f.index));
                        } else {
                            return std::move(when_any_f.tasks[when_any_f.index]);
                        }
                    };
                    auto nth_future = get_nth_future(w);
                    if constexpr (when_any_element) {
                        return continuation(std::forward<PrefixArgs>(prefix_args)..., std::move(nth_future));
                    } else if constexpr (when_any_unwrap_element) {
                        return continuation(std::forward<PrefixArgs>(prefix_args)..., std::move(nth_future.get()));
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

    /// \brief Find the result of unwrap and continue or return unwrapping_failure_t if expression is not well-formed
    template <class Future, class Function, class = void> struct result_of_unwrap {
        using type = unwrapping_failure_t;
    };

    template <class Future, class Function>
    struct result_of_unwrap<
        Future, Function,
        std::void_t<decltype(unwrap_and_continue(std::declval<Future>(), std::declval<Function>()))>> {
        using type = decltype(unwrap_and_continue(std::declval<Future>(), std::declval<Function>()));
    };

    template <class Future, class Function>
    using result_of_unwrap_t = typename result_of_unwrap<Future, Function>::type;

    /// \brief Find the result of unwrap and continue with token or return unwrapping_failure_t otherwise
    template <class Future, class Function, class = void> struct result_of_unwrap_with_token {
        using type = unwrapping_failure_t;
    };

    template <class Future, class Function>
    struct result_of_unwrap_with_token<
        Future, Function,
        std::void_t<decltype(unwrap_and_continue(std::declval<Future>(), std::declval<Function>(),
                                                 std::declval<stop_token>()))>> {
        using type =
            decltype(unwrap_and_continue(std::declval<Future>(), std::declval<Function>(), std::declval<stop_token>()));
    };

    template <class Future, class Function>
    using result_of_unwrap_with_token_t = typename result_of_unwrap_with_token<Future, Function>::type;

    template <typename Function, typename Future> struct unwrap_traits {
        // The return type of unwrap and continue function
        using unwrap_result_no_token_type = result_of_unwrap_t<Future, Function>;
        using unwrap_result_with_token_type = result_of_unwrap_with_token_t<Future, Function>;

        // Whether the continuation expects a token
        static constexpr bool is_valid_without_stop_token =
            !std::is_same_v<unwrap_result_no_token_type, unwrapping_failure_t>;
        static constexpr bool is_valid_with_stop_token =
            !std::is_same_v<unwrap_result_with_token_type, unwrapping_failure_t>;

        // Whether the continuation is valid
        static constexpr bool is_valid = is_valid_without_stop_token || is_valid_with_stop_token;

        // The result type of unwrap and continue for the valid version, with or without token
        using result_value_type =
            std::conditional_t<is_valid_with_stop_token, unwrap_result_with_token_type, unwrap_result_no_token_type>;

        // Stop token for the continuation function
        constexpr static bool continuation_expects_stop_token = is_valid_with_stop_token;

        // Check if the stop token should be inherited from previous future
        constexpr static bool previous_future_has_stop_token = has_stop_token_v<Future>;
        constexpr static bool previous_future_is_shared = is_shared_future_v<Future>;
        constexpr static bool inherit_stop_token = previous_future_has_stop_token && (!previous_future_is_shared);

        // Continuation future should have stop token
        constexpr static bool after_has_stop_token = is_valid_with_stop_token || inherit_stop_token;

        // The result type of unwrap and continue for the valid version, with or without token
        using result_future_type =
            std::conditional_t<after_has_stop_token, jcfuture<result_value_type>, cfuture<result_value_type>>;
    };

    /// \brief The result we get from the `then` function
    /// - If after function expects a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with shared stop source
    ///   - Otherwise:                                      return jcfuture with new stop source
    /// - If after function does not expect a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with shared stop source
    ///   - Otherwise:                                      return cfuture with no stop source
    template <typename Function, typename Future> struct result_of_then {
        using type = typename unwrap_traits<Function, Future>::result_future_type;
    };

    template <typename Function, typename Future>
    using result_of_then_t = typename result_of_then<Function, Future>::type;

    /// \brief A trait to validate whether a Function can be continuation to a future
    template <class Function, class Future>
    using is_valid_continuation = std::bool_constant<unwrap_traits<Function, Future>::is_valid>;

    template <class Function, class Future>
    constexpr bool is_valid_continuation_v = is_valid_continuation<Function, Future>::value;

    // Wrap implementation in empty struct to facilitate friends
    struct internal_then_functor {

        /// \brief Make an appropriate stop source for the continuation
        template <typename Function, class Future>
        static stop_source make_continuation_stop_source(const Future &before, const Function &) {
            using traits = unwrap_traits<Function, Future>;
            if constexpr (traits::after_has_stop_token) {
                if constexpr (traits::inherit_stop_token && (!traits::continuation_expects_stop_token)) {
                    // condition 1: continuation shares token
                    return before.get_stop_source();
                } else {
                    // condition 2: continuation has new token
                    return {};
                }
            } else {
                // condition 3: continuation has no token
                return stop_source(nostopstate);
            }
        }

        /// \brief Maybe copy the previous continuations source
        template <class Future> static continuations_source copy_continuations_source(const Future &before) {
            constexpr bool before_is_lazy_continuable = is_lazy_continuable_v<Future>;
            if constexpr (before_is_lazy_continuable) {
                return before.get_continuations_source();
            } else {
                return continuations_source(nocontinuationsstate);
            }
        }

        /// \brief Create a tuple with the arguments for unwrap and continue
        template <typename Function, class Future>
        static decltype(auto) make_unwrap_args_tuple(Future &&before_future, Function &&continuation_function,
                                                     stop_token st) {
            using traits = unwrap_traits<Function, Future>;
            if constexpr (!traits::is_valid_with_stop_token) {
                return std::make_tuple(std::forward<Future>(before_future),
                                       std::forward<Function>(continuation_function));
            } else {
                return std::make_tuple(std::forward<Future>(before_future),
                                       std::forward<Function>(continuation_function), st);
            }
        }

        struct fulfill_promise_handle {

        };

        template <typename Executor, typename Function, class Future
#ifndef FUTURES_DOXYGEN
                  ,
                  std::enable_if_t<is_executor_v<Executor> && !is_executor_v<Function> && !is_executor_v<Future> &&
                                       is_future_v<Future> && is_valid_continuation_v<Function, Future>,
                                   int> = 0
#endif
                  >
        result_of_then_t<Function, Future> operator()(const Executor &ex, Future &&before, Function &&after) const {
            using traits = unwrap_traits<Function, Future>;

            // Shared sources
            stop_source ss = make_continuation_stop_source(before, after);
            detail::continuations_source after_continuations;
            continuations_source before_cs = copy_continuations_source(before);

            // Set up shared state (packaged task contains unwrap and continue instead of after)
            promise<typename traits::result_value_type> p;
            typename traits::result_future_type result{p.template get_future<typename traits::result_future_type>()};
            result.set_continuations_source(after_continuations);
            if constexpr (traits::after_has_stop_token) {
                result.set_stop_source(ss);
            }
            // Set the complete executor task, using result to fulfill the promise, and running continuations
            auto fulfill_promise = [p = std::move(p),                                           // task and shared state
                                    before_future = move_or_copy(std::forward<Future>(before)), // the previous future
                                    continuation = std::forward<Function>(after), // the continuation function
                                    after_continuations,                          // continuation source for after
                                    token = ss.get_token()                        // maybe shared stop token
            ]() mutable {
                try {
                    if constexpr (std::is_same_v<typename traits::result_value_type, void>) {
                        if constexpr (traits::is_valid_with_stop_token) {
                            detail::unwrap_and_continue(before_future, continuation, token);
                            p.set_value();
                        } else {
                            detail::unwrap_and_continue(before_future, continuation);
                            p.set_value();
                        }
                    } else {
                        if constexpr (traits::is_valid_with_stop_token) {
                            p.set_value(detail::unwrap_and_continue(before_future, continuation, token));
                        } else {
                            p.set_value(detail::unwrap_and_continue(before_future, continuation));
                        }
                    }
                } catch (...) {
                    p.set_exception(std::current_exception());
                }
                after_continuations.request_run();
            };

            // Move function to shared location because executors require handles to be copy constructable
            auto fulfill_promise_ptr = std::make_shared<decltype(fulfill_promise)>(std::move(fulfill_promise));
            auto copyable_handle = [fulfill_promise_ptr]() { (*fulfill_promise_ptr)(); };

            // Fire-and-forget: Post a handle running the complete continuation function to the executor
            if constexpr (is_lazy_continuable_v<Future>) {
                // Attach continuation to previous future.
                // - Continuation is posted when previous is done
                // - Continuation is posted immediately if previous is already done
                before_cs.emplace_continuation(
                    ex, [h = std::move(copyable_handle), ex]() { asio::post(ex, std::move(h)); });
            } else {
                // We defer the task in the executor because the input doesn't have lazy continuations.
                // The executor will take care of this running later, so we don't need polling.
                asio::defer(ex, std::move(copyable_handle));
            }

            return result;
        }
    };
    constexpr internal_then_functor internal_then;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_CONTINUATION_UNWRAP_H
