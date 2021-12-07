//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_IS_FUTURE_CONTINUATION_H
#define FUTURES_IS_FUTURE_CONTINUATION_H

#include <futures/algorithm/detail/traits/range/range/concepts.hpp>
#include <futures/adaptor/detail/traits/is_when_any_result.h>

#include <futures/config/small_vector_include.h>

#include <futures/futures/traits/is_future.h>
#include <futures/futures/traits/unwrap_future.h>

#include <futures/adaptor/detail/traits/is_tuple.h>

/// \file Check if a function can be a invoked with the return type of a future
/// This handles two main cases:
/// 1) future<void>, or                          -> the function needs to be invocable without arguments
/// 2) future<T>                                 -> the function needs to be invocable with T or unwrapped T
///    i) future<T>, or                          -> the function needs to be invocable with T
///    ii) future<std::tuple<T>>, or             -> the function needs to be invocable with tuple value
///    iii) future<std::tuple<future<T>...>>, or -> the function needs to be invocable with tuple futures
///    iv) future<std::future<T>>                -> the function needs to be invocable with T
///    v...) see below

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    template <typename Sequence> struct when_any_result;

    namespace detail {
        /// \brief Check if a function can be invoked after a future with no result, like future<void>
        /// ex: function() from result of future<void>
        template <class Function, class Future, class... Args>
        using is_void_continuation =
            std::conjunction<std::negation<is_future<Function>>, // Callable cannot be a future forbid ambiguity in
                                                                 // overloads
                             is_future<Future>,                  // Future needs to be a future to forbid bad overloads
                             std::is_same<unwrap_future_t<Future>, void>, // T in future<T> is void
                             std::is_invocable<Function, Args...> // we can invoke the function with no arguments
                             >;

        template <class Function, class Future, class... Args>
        constexpr bool is_void_continuation_v = is_void_continuation<Function, Future, Args...>::value;

        /// \brief Check if a function can be invoked directly with the result type from future
        /// - function(T) as continuation to future<T>
        /// - function(const T) as continuation to future<T>
        /// - function(const T&) as continuation to future<T>
        template <class Function, class Future, class... Args>
        using is_direct_continuation = std::is_invocable<Function, Args..., unwrap_future_t<Future>>;

        template <class Function, class Future, class... Args>
        constexpr bool is_direct_continuation_v = is_direct_continuation<Function, Future, Args...>::value;

        /// \brief Check if a function can be invoked with lvalue result type from future
        /// - function(T&) as continuation to future<T>
        template <class Function, class Future, class... Args>
        using is_lvalue_continuation =
            std::is_invocable<Function, Args..., std::add_lvalue_reference_t<unwrap_future_t<Future>>>;

        template <class Function, class Future, class... Args>
        constexpr bool is_lvalue_continuation_v = is_lvalue_continuation<Function, Future, Args...>::value;

        /// \brief Check if a function can be invoked with rvalue result type from future
        /// - function(T&&) as continuation to future<T>
        template <class Function, class Future, class... Args>
        using is_rvalue_continuation =
            std::is_invocable<Function, Args..., std::add_rvalue_reference_t<unwrap_future_t<Future>>>;

        template <class Function, class Future, class... Args>
        constexpr bool is_rvalue_continuation_v = is_rvalue_continuation<Function, Future, Args...>::value;

        /// \brief Check if a function can be invoked with an unwrapped unwrapped future of future
        /// In the few cases where we might need to unwrap the future twice
        /// - function(T) as continuation to future<future<T>>
        /// - function(const T) as continuation to future<future<T>>
        /// - function(const T&) as continuation to future<future<T>>
        template <class Function, class Future, class... Args>
        using is_double_unwrap_continuation =
            std::conjunction<is_future<Future>, is_future<unwrap_future_t<Future>>,
                             std::is_invocable<Function, Args..., unwrap_future_t<unwrap_future_t<Future>>>>;

        template <class Function, class Future, class... Args>
        constexpr bool is_double_unwrap_continuation_v =
            is_double_unwrap_continuation<Function, Future, Args...>::value;

        /// \brief Check if a function can be invoked with the elements of a tuple, as in std::apply
        template <typename Function, typename ArgsTuple, typename RealTuple, typename... Args>
        struct is_tuple_explode_invocable : std::false_type {};

        template <typename Function, class... Args, typename... TupleArgs>
        struct is_tuple_explode_invocable<Function, std::tuple<Args...>, std::tuple<TupleArgs...>>
            : std::is_invocable<Function, Args..., TupleArgs...> {};

        /// \brief Check if a function can be invoked with an unwrapped tuple from future<tuple<...>>
        /// - function(T0, T1, ...) as continuation to future<tuple<T0, T1,...>>
        template <class Function, class Future, class... Args>
        using is_tuple_explode_continuation =
            std::conjunction<is_tuple<unwrap_future_t<Future>>,
                             is_tuple_explode_invocable<Function, std::tuple<Args...>, unwrap_future_t<Future>>>;

        template <class Function, class Future, class... Args>
        constexpr bool is_tuple_explode_continuation_v =
            is_tuple_explode_continuation<Function, Future, Args...>::value;

        /// \brief Check if all template parameters are futures
        template <class...> struct are_futures : std::true_type {};
        template <class B1> struct are_futures<B1> : is_future<B1> {};
        template <class B1, class... Bn>
        struct are_futures<B1, Bn...> : std::conditional_t<is_future_v<B1>, are_futures<Bn...>, std::false_type> {};

        /// \brief Check if a element is a tuple of futures
        /// Something like tuple<future<T>, future<T2>, ...>
        template <typename T> struct is_tuple_of_futures : std::false_type {};
        template <typename... Args> struct is_tuple_of_futures<std::tuple<Args...>> : are_futures<Args...> {};

        /// \brief Check if a function can be invoked with the elements of a tuple, as in std::apply
        template <typename Function, typename ArgsTuple, typename RealTuple>
        struct is_tuple_unwrap_invocable : std::false_type {};
        template <typename Function, typename... Args, typename... TupleArgs>
        struct is_tuple_unwrap_invocable<Function, std::tuple<Args...>, std::tuple<TupleArgs...>>
            : std::is_invocable<Function, Args..., unwrap_future_t<TupleArgs>...> {};

        /// \brief Check if a function can be invoked with unwrapped futures of a tuple (as with std::apply to futures)
        /// Something like function(T1, T2, ...) as continuation to  tuple<future<T>, future<T2>, ...>
        /// - function(T0, T1, ...) as continuation to future<tuple<future<T0>, future<T1>,...>>
        template <class Function, class Future, class... Args>
        using is_tuple_unwrap_continuation = std::conjunction<
            is_tuple_of_futures<unwrap_future_t<Future>>, // T in future<T> is tuple<future<T1>, future<T2>, ...>
            is_tuple_unwrap_invocable<Function, std::tuple<Args...>, unwrap_future_t<Future>> // we can invoke Function
                                                                                              // with T1, T2,...
            >;

        template <class Function, class Future, class... Args>
        constexpr bool is_tuple_unwrap_continuation_v = is_tuple_unwrap_continuation<Function, Future, Args...>::value;

        /// \brief Check if a element is a range of futures
        /// Something like vector<future<T>>
        template <typename T, class Enable = void> struct is_range_of_futures : std::false_type {};
        template <typename Range>
        struct is_range_of_futures<Range, std::enable_if_t<futures::detail::range<std::decay_t<Range>>>>
            : is_future<typename Range::value_type> {};

        /// \brief Check if a function can be invoked with the elements of a range
        /// This assumes the continuation expects a small vector, because it's what seems reasonable
        /// for this library and it's what when_all futures store internally.
        /// We could adapt this to other kinds of ranges/container, but supporting every kind of container
        /// is unfeasible because we actually need to create and set up the container with the unwrapped
        /// results before forwarding it to the continuation function.
        template <typename Function, typename T, typename... Args>
        struct is_vector_unwrap_invocable : std::false_type {};
        template <typename Function, typename FT, typename... Args>
        struct is_vector_unwrap_invocable<Function, futures::small_vector<FT>, Args...>
            : std::disjunction<std::is_invocable<Function, Args..., futures::small_vector<unwrap_future_t<FT>>>,
                               std::is_invocable<Function, Args...,
                                                 std::add_lvalue_reference_t<futures::small_vector<unwrap_future_t<FT>>>>> {};

        /// \brief Check if a function can be invoked with unwrapped futures of a range
        /// - Something like function(futures::small_vector<T>) as continuation to futures::small_vector<future<T>>
        template <class Function, class Future, class... Args>
        using is_vector_unwrap_continuation = std::conjunction<
            is_range_of_futures<unwrap_future_t<Future>>, // T in future<T> is tuple<future<T1>, future<T2>, ...>
            is_vector_unwrap_invocable<Function, unwrap_future_t<Future>, Args...> // we can invoke Function with T1,
                                                                                   // T2,...
            >;

        template <class Function, class Future, class... Args>
        constexpr bool is_vector_unwrap_continuation_v =
            is_vector_unwrap_continuation<Function, Future, Args...>::value;

        /// Check if type is a when_any_result where the sequence type is a tuple
        template <typename> struct is_tuple_when_any_result : std::false_type {};
        template <typename Sequence> struct is_tuple_when_any_result<when_any_result<Sequence>> : is_tuple<Sequence> {};
        template <class T> constexpr bool is_tuple_when_any_result_v = is_tuple_when_any_result<T>::value;

        /// \brief Check if a function can be invoked with the decomposed elements of a tuple_when_any_result
        template <typename Function, typename ArgsTuple, typename T>
        struct is_index_and_sequence_invocable : std::false_type {};

        template <typename Function, typename... ArgsTupleArgs, typename... Args>
        struct is_index_and_sequence_invocable<Function, std::tuple<ArgsTupleArgs...>,
                                               when_any_result<std::tuple<Args...>>>
            : std::is_invocable<Function, ArgsTupleArgs..., size_t, std::tuple<Args...>> {};

        template <typename Function, typename Sequence, typename... ArgsTuple>
        struct is_index_and_sequence_invocable<Function, std::tuple<ArgsTuple...>, when_any_result<Sequence>>
            : std::is_invocable<Function, ArgsTuple..., size_t, Sequence> {};

        /// \brief Check if a function can be invoked with unwrapped when_any_result as two arguments
        /// Something like function(size_t, tuple<future<T1>, future<T2>, ...>) as continuation to
        /// when_any_result<tuple<future<T>, future<T2>, ...>>
        template <class Function, class Future, class... Args>
        using is_when_any_split_continuation =
            std::conjunction<is_when_any_result<unwrap_future_t<Future>>,
                             is_index_and_sequence_invocable<Function, std::tuple<Args...>, unwrap_future_t<Future>>>;

        template <class Function, class Future, class... Args>
        constexpr bool is_when_any_split_continuation_v =
            is_when_any_split_continuation<Function, Future, Args...>::value;

        /// \brief Check if a function can be invoked with the decomposed elements of a tuple_when_any_result
        template <typename Function, typename ArgsTuple, typename Tuple>
        struct is_when_any_explode_invocable : std::false_type {};
        template <typename Function, typename... FuncArgs, typename... TupleArgs>
        struct is_when_any_explode_invocable<Function, std::tuple<FuncArgs...>,
                                             when_any_result<std::tuple<TupleArgs...>>>
            : std::is_invocable<Function, FuncArgs..., size_t, TupleArgs...> {};

        /// \brief Check if a function can be invoked with unwrapped when_any_result futures
        /// Something like function(size_t, future<T1>, future<T2>, ...) as continuation to
        /// when_any_result<tuple<future<T>, future<T2>, ...>>
        /// We cannot further unwrap the individual futures here because they are not ready yet
        /// We can only unwrap them if 1) all have the same type or 2) all have different types and are
        /// converted into a std::variant. One could also implement a version a std::variant whose types
        /// are tagged so we could have repeated types in it, but this is out of the scope of this module.
        template <class Function, class Future, class... Args>
        using is_when_any_explode_continuation =
            std::conjunction<is_tuple_when_any_result<unwrap_future_t<Future>>,
                             is_when_any_explode_invocable<Function, std::tuple<Args...>, unwrap_future_t<Future>>>;

        template <class Function, class Future, class... Args>
        constexpr bool is_when_any_explode_continuation_v =
            is_when_any_explode_continuation<Function, Future, Args...>::value;

        /// \brief Check if all types in a list are the same type
        template <class...> struct are_same : std::true_type {};
        template <class B1> struct are_same<B1> : std::true_type {};
        template <class B1, class B2> struct are_same<B1, B2> : std::is_same<B1, B2> {};
        template <class B1, class B2, class... Bn>
        struct are_same<B1, B2, Bn...>
            : std::conditional_t<std::is_same_v<B1, B2>, are_same<B2, Bn...>, std::false_type> {};

        /// \brief Check if a function can be invoked with the single element from a tuple_when_any_result
        template <typename Function, typename ArgsTuple, typename WhenAnyFuture>
        struct is_when_any_element_invocable : std::false_type {};

        template <typename Function, typename SequenceFuture1, typename... SequenceFutures, typename... Args>
        struct is_when_any_element_invocable<Function, std::tuple<Args...>,
                                             when_any_result<std::tuple<SequenceFuture1, SequenceFutures...>>>
            : std::conjunction<are_same<SequenceFuture1, SequenceFutures...>,
                               std::is_invocable<Function, Args..., SequenceFuture1>> {};

        template <typename Function, typename Sequence, typename... Args>
        struct is_when_any_element_invocable<Function, std::tuple<Args...>, when_any_result<Sequence>>
            : std::is_invocable<Function, Args..., typename Sequence::value_type> {};

        /// \brief Check if a function can be invoked with unwrapped when_any_result futures
        /// Something like function(T) as continuation to when_any_result<tuple<future<T>, future<T2>, ...>>
        /// We cannot further unwrap the individual futures here because they are not ready yet
        /// We can only unwrap them if 1) all have the same type or 2) all have different types and are
        /// converted into a std::variant. One could also implement a version a std::variant whose types
        /// are tagged so we could have repeated types in it, but this is out of the scope of this module.
        template <class Function, class Future, class... Args>
        using is_when_any_element_continuation =
            std::conjunction<is_when_any_result<unwrap_future_t<Future>>,
                             is_when_any_element_invocable<Function, std::tuple<Args...>, unwrap_future_t<Future>>>;

        template <class Function, class Future, class... Args>
        constexpr bool is_when_any_element_continuation_v =
            is_when_any_element_continuation<Function, Future, Args...>::value;

        /// \brief Check if a function can be invoked with the single element from a tuple_when_any_result
        template <typename Function, typename ArgsTuple, typename WhenAnyFuture>
        struct is_when_any_unwrap_invocable : std::false_type {};
        template <typename Function, typename Arg1, typename... Args, typename... FuncArgs>
        struct is_when_any_unwrap_invocable<Function, std::tuple<FuncArgs...>,
                                            when_any_result<std::tuple<Arg1, Args...>>>
            : std::conjunction<std::is_invocable<Function, FuncArgs..., unwrap_future_t<Arg1>>,
                               are_same<unwrap_future_t<Arg1>, unwrap_future_t<Args>...>> {};
        template <typename Function, typename Sequence, typename... FuncArgs>
        struct is_when_any_unwrap_invocable<Function, std::tuple<FuncArgs...>, when_any_result<Sequence>>
            : std::is_invocable<Function, FuncArgs..., unwrap_future_t<typename Sequence::value_type>> {};

        /// \brief Check if a function can be invoked with unwrapped when_any_result futures
        /// Something like function(size_t, future<T1>, future<T2>, ...) as continuation to
        /// when_any_result<tuple<future<T>, future<T2>, ...>>
        /// We cannot further unwrap the individual futures here because they are not ready yet
        /// We can only unwrap them if 1) all have the same type or 2) all have different types and are
        /// converted into a std::variant. One could also implement a version a std::variant whose types
        /// are tagged so we could have repeated types in it, but this is out of the scope of this module.
        template <class Function, class Future, class... Args>
        using is_when_any_unwrap_continuation =
            std::conjunction<is_when_any_result<unwrap_future_t<Future>>,
                             is_when_any_unwrap_invocable<Function, std::tuple<Args...>, unwrap_future_t<Future>>>;

        template <class Function, class Future, class... Args>
        constexpr bool is_when_any_unwrap_continuation_v =
            is_when_any_unwrap_continuation<Function, Future, Args...>::value;
    } // namespace detail

    /// \brief Check if a function can be a continuation to a future
    /// This goes through all the unwrapping rules we allow for the then function
    /// The extra args can be used to define arguments that come BEFORE the unwrapped
    /// future result. In practice, this is use to check if the continuation has a
    /// stop token, but it's best to make this easier to extend later.
    template <class Function, class Future, class... Args>
    using is_future_continuation = std::disjunction<
        // Common continuations for basic_future
        detail::is_void_continuation<Function, Future, Args...>,
        detail::is_direct_continuation<Function, Future, Args...>,
        detail::is_lvalue_continuation<Function, Future, Args...>,
        detail::is_rvalue_continuation<Function, Future, Args...>,
        detail::is_double_unwrap_continuation<Function, Future, Args...>,

        // Common continuations for when_all
        detail::is_tuple_explode_continuation<Function, Future, Args...>,
        detail::is_tuple_unwrap_continuation<Function, Future, Args...>,
        detail::is_vector_unwrap_continuation<Function, Future, Args...>,

        // Common continuations for when_any
        detail::is_when_any_split_continuation<Function, Future, Args...>,
        detail::is_when_any_explode_continuation<Function, Future, Args...>,
        detail::is_when_any_element_continuation<Function, Future, Args...>,
        detail::is_when_any_unwrap_continuation<Function, Future, Args...>>;

    template <class Function, class Future, class... Args>
    constexpr bool is_future_continuation_v = is_future_continuation<Function, Future, Args...>::value;

    /// \brief Determines the result of the continuation Function applied to the previous Future result
    /// This considers the given Function input types and the order of unwrapping rules to determine which
    /// continuation would be invoked. Thus, for each enable_if, we have to ensure it's a valid continuation
    /// for that future but not a valid continuation for the previous futures. This should be easy, if it
    /// weren't for functions that accept auto.
    template <class Function, class Future, class ArgsTuple, class Enable = void> struct continuation_result {
        using type = void;
    };

    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<Function, Future, std::tuple<FuncArgs...>,
                               std::enable_if_t<detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
        using type = std::invoke_result_t<Function, FuncArgs...>;
    };

    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<Function, Future, std::tuple<FuncArgs...>,
                               std::enable_if_t<detail::is_direct_continuation_v<Function, Future, FuncArgs...> &&
                                                not detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
        using type = std::invoke_result_t<Function, FuncArgs..., unwrap_future_t<Future>>;
    };

    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<Function, Future, std::tuple<FuncArgs...>,
                               std::enable_if_t<detail::is_lvalue_continuation_v<Function, Future, FuncArgs...> &&
                                                not detail::is_direct_continuation_v<Function, Future, FuncArgs...> &&
                                                not detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
        using type = std::invoke_result_t<Function, FuncArgs..., std::add_lvalue_reference_t<unwrap_future_t<Future>>>;
    };

    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<Function, Future, std::tuple<FuncArgs...>,
                               std::enable_if_t<detail::is_rvalue_continuation_v<Function, Future, FuncArgs...> &&
                                                not detail::is_direct_continuation_v<Function, Future, FuncArgs...> &&
                                                not detail::is_lvalue_continuation_v<Function, Future, FuncArgs...> &&
                                                not detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
        using type = std::invoke_result_t<Function, FuncArgs..., std::add_rvalue_reference_t<unwrap_future_t<Future>>>;
    };

    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<
        Function, Future, std::tuple<FuncArgs...>,
        std::enable_if_t<detail::is_double_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_rvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_direct_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_lvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
        using type = std::invoke_result_t<Function, FuncArgs..., unwrap_future_t<unwrap_future_t<Future>>>;
    };

    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<
        Function, Future, std::tuple<FuncArgs...>,
        std::enable_if_t<detail::is_tuple_explode_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_double_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_rvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_direct_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_lvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
      private:
        template <typename Tuple> struct tuple_explosion_result { using type = void; };

        template <typename... Args>
        struct tuple_explosion_result<std::tuple<Args...>> : std::invoke_result<Function, FuncArgs..., Args...> {};

        template <typename Tuple> using tuple_explosion_result_t = typename tuple_explosion_result<Tuple>::type;

      public:
        using type = tuple_explosion_result_t<unwrap_future_t<Future>>;
    };

    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<
        Function, Future, std::tuple<FuncArgs...>,
        std::enable_if_t<detail::is_tuple_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_tuple_explode_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_double_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_rvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_direct_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_lvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
      private:
        template <typename Tuple> struct tuple_unwrap_result { using type = void; };

        template <typename... Args>
        struct tuple_unwrap_result<std::tuple<Args...>>
            : std::invoke_result<Function, FuncArgs..., unwrap_future_t<Args>...> {};

        template <typename Tuple> using tuple_unwrap_result_t = typename tuple_unwrap_result<Tuple>::type;

      public:
        using type = tuple_unwrap_result_t<unwrap_future_t<Future>>;
    };

    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<
        Function, Future, std::tuple<FuncArgs...>,
        std::enable_if_t<detail::is_vector_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_tuple_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_tuple_explode_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_double_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_rvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_direct_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_lvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
        template <typename T> struct is_vector_unwrap_invocable : std::false_type {};

      private:
        template <typename Vector, class Enable = void> struct vector_unwrap_result { using type = void; };

        template <typename Arg>
        struct vector_unwrap_result<
            futures::small_vector<Arg>,
            std::enable_if_t<std::is_invocable_v<Function, FuncArgs..., futures::small_vector<unwrap_future_t<Arg>>>>> {
            using type = std::invoke_result_t<Function, FuncArgs..., futures::small_vector<unwrap_future_t<Arg>>>;
        };

        template <typename Arg>
        struct vector_unwrap_result<
            futures::small_vector<Arg>,
            std::enable_if_t<not std::is_invocable_v<Function, FuncArgs..., futures::small_vector<unwrap_future_t<Arg>>>>> {
            using type = std::invoke_result_t<Function, FuncArgs...,
                                              std::add_lvalue_reference_t<futures::small_vector<unwrap_future_t<Arg>>>>;
        };

        template <typename Vector> using vector_unwrap_result_t = typename vector_unwrap_result<Vector>::type;

      public:
        using type = vector_unwrap_result_t<unwrap_future_t<Future>>;
    };

    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<
        Function, Future, std::tuple<FuncArgs...>,
        std::enable_if_t<detail::is_when_any_split_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_vector_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_tuple_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_tuple_explode_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_double_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_rvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_direct_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_lvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
      private:
        template <typename WhenAny> struct when_any_split_result { using type = void; };

        template <typename... TupleArgs>
        struct when_any_split_result<when_any_result<std::tuple<TupleArgs...>>>
            : std::invoke_result<Function, FuncArgs..., size_t, std::tuple<TupleArgs...>> {};

        template <typename Sequence>
        struct when_any_split_result<when_any_result<Sequence>>
            : std::invoke_result<Function, FuncArgs..., size_t, Sequence> {};

        template <typename WhenAny> using when_any_split_result_t = typename when_any_split_result<WhenAny>::type;

      public:
        using type = when_any_split_result_t<unwrap_future_t<Future>>;
    };

    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<
        Function, Future, std::tuple<FuncArgs...>,
        std::enable_if_t<detail::is_when_any_explode_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_when_any_split_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_vector_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_tuple_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_tuple_explode_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_double_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_rvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_direct_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_lvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
      private:
        template <typename WhenAny> struct when_any_explode_result { using type = void; };

        template <typename... TupleArgs>
        struct when_any_explode_result<when_any_result<std::tuple<TupleArgs...>>>
            : std::invoke_result<Function, FuncArgs..., size_t, TupleArgs...> {};

        template <typename WhenAny> using when_any_explode_result_t = typename when_any_explode_result<WhenAny>::type;

      public:
        using type = when_any_explode_result_t<unwrap_future_t<Future>>;
    };

    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<
        Function, Future, std::tuple<FuncArgs...>,
        std::enable_if_t<detail::is_when_any_element_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_when_any_explode_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_when_any_split_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_vector_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_tuple_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_tuple_explode_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_double_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_rvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_direct_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_lvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
      private:
        template <typename WhenAny> struct when_any_element_result { using type = void; };

        template <typename TupleArg0, typename... TupleArgs>
        struct when_any_element_result<when_any_result<std::tuple<TupleArg0, TupleArgs...>>>
            : std::invoke_result<Function, FuncArgs..., TupleArg0> {};

        template <typename Sequence>
        struct when_any_element_result<when_any_result<Sequence>>
            : std::invoke_result<Function, FuncArgs..., typename Sequence::value_type> {};

        template <typename WhenAny> using when_any_element_result_t = typename when_any_element_result<WhenAny>::type;

      public:
        using type = when_any_element_result_t<unwrap_future_t<Future>>;
    };

    /// Determine result of @ref Function with @ref FuncArgs used as a continuation to @ref Future
    ///
    /// \tparam Function Function to come after the future
    /// \tparam Future Future object type
    /// \tparam FuncArgs Function arguments
    template <class Function, class Future, class... FuncArgs>
    struct continuation_result<
        Function, Future, std::tuple<FuncArgs...>,
        std::enable_if_t<detail::is_when_any_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_when_any_element_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_when_any_explode_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_when_any_split_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_vector_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_tuple_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_tuple_explode_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_double_unwrap_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_rvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_direct_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_lvalue_continuation_v<Function, Future, FuncArgs...> &&
                         not detail::is_void_continuation_v<Function, Future, FuncArgs...>>> {
      private:
        template <typename WhenAny> struct when_any_unwrap_result { using type = void; };

        template <typename TupleArg0, typename... TupleArgs>
        struct when_any_unwrap_result<when_any_result<std::tuple<TupleArg0, TupleArgs...>>>
            : std::invoke_result<Function, FuncArgs..., unwrap_future_t<TupleArg0>> {};

        template <typename Sequence>
        struct when_any_unwrap_result<when_any_result<Sequence>>
            : std::invoke_result<Function, FuncArgs..., unwrap_future_t<typename Sequence::value_type>> {};

        template <typename WhenAny> using when_any_unwrap_result_t = typename when_any_unwrap_result<WhenAny>::type;

      public:
        using type = when_any_unwrap_result_t<unwrap_future_t<Future>>;
    };

    /// Determine result of @ref Function with @ref FuncArgs used as a continuation to @ref Future
    ///
    /// \tparam Function Function to come after the future
    /// \tparam Future Future object type
    /// \tparam FuncArgs Function arguments
    template <class Function, class Future, class FuncArgsTuple>
    using continuation_result_t = typename continuation_result<Function, Future, FuncArgsTuple>::type;

    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_IS_FUTURE_CONTINUATION_H
