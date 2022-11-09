//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_CONTINUE_INVOKE_TAG_HPP
#define FUTURES_ADAPTOR_DETAIL_CONTINUE_INVOKE_TAG_HPP

#include <futures/config.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/detail/container/small_vector.hpp>
#include <futures/detail/traits/future_value.hpp>
#include <futures/adaptor/detail/when_any.hpp>
#include <futures/detail/deps/boost/mp11/algorithm.hpp>
#include <futures/detail/deps/boost/mp11/function.hpp>
#include <futures/detail/deps/boost/mp11/list.hpp>

namespace futures::detail {
    // --------------------------------------------------------------
    // continue tags for tag-dispatching and identifying each unwrapping type
    //

    struct continue_tags {
        struct no_unwrap {};
        struct no_input {};
        struct rvalue_unwrap {};
        struct double_unwrap {};
        struct deepest_unwrap {};
        struct tuple_explode_unwrap {};
        struct futures_tuple_double_unwrap {};
        struct futures_tuple_deepest_unwrap {};
        struct futures_range_double_unwrap {};
        struct futures_range_deepest_unwrap {};
        struct when_any_split_unwrap {};
        struct when_any_explode_unwrap {};
        struct when_any_tuple_element_unwrap {};
        struct when_any_range_element_unwrap {};
        struct when_any_tuple_double_unwrap {};
        struct when_any_tuple_deepest_unwrap {};
        struct when_any_range_double_unwrap {};
        struct when_any_range_deepest_unwrap {};
        struct failure {};
    };

    // --------------------------------------------------------------
    // the result type we get when invoking each continuation
    //

    template <
        class Tag,
        class Future,
        class PrefixList,
        class Function,
        class = void>
    struct continue_invoke_traits {
        using valid = std::false_type;
        using result = mp_identity<continue_tags::failure>;
    };

    // values are generally moved to continuation functions
    template <class T>
    using rvalue_t = std::add_rvalue_reference_t<std::decay_t<T>>;

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::no_unwrap,
        Future,
        PrefixList,
        Function> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<mp_list<Function>, PrefixList, mp_list<Future>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::no_input,
        Future,
        PrefixList,
        Function> {
    private:
        template <template <class...> class F>
        using invoke_expr
            = mp_apply<F, mp_append<mp_list<Function>, PrefixList>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::rvalue_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && mp_valid<
                std::is_invocable,
                mp_append<
                    mp_list<Function>,
                    PrefixList,
                    mp_list<rvalue_t<future_value_t<Future>>>>>::value>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<rvalue_t<future_value_t<Future>>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::double_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_future_v<future_value_t<Future>>>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<rvalue_t<future_value_t<future_value_t<Future>>>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    // Unwrap the value of a future to the deepest level
    template <class T, class = void>
    struct unwrap_future {
        using type = T;
    };

    template <class T>
    struct unwrap_future<T, std::enable_if_t<futures::is_future_v<T>>> {
        using type = typename unwrap_future<futures::future_value_t<T>>::type;
    };

    template <class F>
    using unwrap_future_t = typename unwrap_future<F>::type;

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::deepest_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_future_v<future_value_t<Future>>>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<rvalue_t<unwrap_future_t<Future>>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::tuple_explode_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && mp_similar<std::tuple<>, std::decay_t<future_value_t<Future>>>::
                value>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_transform<rvalue_t, future_value_t<Future>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::futures_tuple_double_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && mp_similar<std::tuple<>, std::decay_t<future_value_t<Future>>>::value
            && mp_all_of<future_value_t<Future>, is_future>::value>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_transform_q<
                    mp_compose<rvalue_t, future_value_t>,
                    future_value_t<Future>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::futures_tuple_deepest_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && mp_similar<std::tuple<>, std::decay_t<future_value_t<Future>>>::value
            && mp_all_of<
                mp_transform<std::decay_t, future_value_t<Future>>,
                is_future>::value>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_transform_q<
                    mp_compose<unwrap_future_t, rvalue_t>,
                    future_value_t<Future>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::futures_range_double_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_range_v<std::decay_t<future_value_t<Future>>>
            && is_future_v<
                std::decay_t<range_value_t<future_value_t<Future>>>>>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<rvalue_t<detail::small_vector<
                    future_value_t<range_value_t<future_value_t<Future>>>>>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::futures_range_deepest_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_range_v<std::decay_t<future_value_t<Future>>>
            && is_future_v<
                std::decay_t<range_value_t<future_value_t<Future>>>>>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<rvalue_t<detail::small_vector<
                    unwrap_future_t<range_value_t<future_value_t<Future>>>>>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::when_any_split_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_when_any_result_v<std::decay_t<future_value_t<Future>>>>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<
                    rvalue_t<typename future_value_t<Future>::size_type>,
                    rvalue_t<typename future_value_t<Future>::sequence_type>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::when_any_explode_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_when_any_result_v<std::decay_t<future_value_t<Future>>>
            && mp_similar<
                std::tuple<>,
                std::decay_t<typename future_value_t<Future>::sequence_type>>::value
            && mp_all_of<
                std::decay_t<typename future_value_t<Future>::sequence_type>,
                is_future>::value>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<rvalue_t<typename future_value_t<Future>::size_type>>,
                mp_transform<
                    rvalue_t,
                    typename future_value_t<Future>::sequence_type>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::when_any_tuple_element_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_when_any_result_v<std::decay_t<future_value_t<Future>>>
            && mp_similar<
                std::tuple<>,
                std::decay_t<typename future_value_t<Future>::sequence_type>>::value
            && mp_all_of<
                mp_transform<
                    std::decay_t,
                    std::decay_t<typename future_value_t<Future>::sequence_type>>,
                is_future>::value
            && mp_same<std::decay_t<
                typename future_value_t<Future>::sequence_type>>::value>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<rvalue_t<std::tuple_element_t<
                    0,
                    typename future_value_t<Future>::sequence_type>>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::when_any_range_element_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_when_any_result_v<std::decay_t<future_value_t<Future>>>
            && is_range_v<std::decay_t<
                typename future_value_t<Future>::sequence_type>>>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<rvalue_t<range_value_t<
                    typename future_value_t<Future>::sequence_type>>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::when_any_tuple_double_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_when_any_result_v<std::decay_t<future_value_t<Future>>>
            && mp_similar<
                std::tuple<>,
                std::decay_t<typename future_value_t<Future>::sequence_type>>::value
            && mp_all_of<
                mp_transform<
                    std::decay_t,
                    typename future_value_t<Future>::sequence_type>,
                is_future>::value
            && mp_same<mp_transform<
                std::decay_t,
                std::decay_t<typename future_value_t<Future>::sequence_type>>>::
                value>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<rvalue_t<future_value_t<std::tuple_element_t<
                    0,
                    typename future_value_t<Future>::sequence_type>>>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::when_any_tuple_deepest_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_when_any_result_v<std::decay_t<future_value_t<Future>>>
            && mp_similar<
                std::tuple<>,
                std::decay_t<typename future_value_t<Future>::sequence_type>>::value
            && mp_all_of<
                mp_transform<
                    std::decay_t,
                    typename future_value_t<Future>::sequence_type>,
                is_future>::value
            && mp_same<mp_transform_q<
                mp_compose<unwrap_future_t, std::decay_t>,
                typename future_value_t<Future>::sequence_type>>::value>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<rvalue_t<unwrap_future_t<std::tuple_element_t<
                    0,
                    typename future_value_t<Future>::sequence_type>>>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::when_any_range_double_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_when_any_result_v<std::decay_t<future_value_t<Future>>>
            && is_range_v<std::decay_t<
                typename future_value_t<Future>::sequence_type>>>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<std::decay_t<future_value_t<range_value_t<
                    typename future_value_t<Future>::sequence_type>>>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    template <class Future, class PrefixList, class Function>
    struct continue_invoke_traits<
        continue_tags::when_any_range_deepest_unwrap,
        Future,
        PrefixList,
        Function,
        std::enable_if_t<
            !continue_invoke_traits<
                continue_tags::no_unwrap,
                Future,
                PrefixList,
                Function>::valid::value
            && is_when_any_result_v<std::decay_t<future_value_t<Future>>>
            && is_range_v<std::decay_t<
                typename future_value_t<Future>::sequence_type>>>> {
    private:
        template <template <class...> class F>
        using invoke_expr = mp_apply<
            F,
            mp_append<
                mp_list<Function>,
                PrefixList,
                mp_list<std::decay_t<unwrap_future_t<range_value_t<
                    typename future_value_t<Future>::sequence_type>>>>>>;
    public:
        using valid = invoke_expr<std::is_invocable>;
        using result = invoke_expr<std::invoke_result>;
    };

    // --------------------------------------------------------------
    // determine continuation tag for a continuation given the specified
    // previous future. The first valid continuation unwrapping is the
    // one the continuation attempts to use
    //

    template <class Future, class Function, class... PrefixArgs>
    struct continue_tag : private continue_tags {
    private:
        template <class Tag>
        using is_valid = typename continue_invoke_traits<
            Tag,
            Future,
            mp_list<PrefixArgs...>,
            Function>::valid;

    public:
        using type = mp_cond<
            /*
             * No unwrapping
             */
            // f(future<R>)
            is_valid<no_unwrap>,
            no_unwrap,

            // f()
            is_valid<no_input>,
            no_input,

            /*
             * Unwrapping a single value
             */
            // f(R), f(R const&), f(R&&)
            is_valid<rvalue_unwrap>,
            rvalue_unwrap,

            /*
             * Nested futures
             */
            // future<future<R>> -> f(R)
            // future<future<future<R>>> -> f(future<R>)
            is_valid<double_unwrap>,
            double_unwrap,

            // future<future<R>> -> f(R)
            // future<future<future<R>>> -> f(R)
            is_valid<deepest_unwrap>,
            deepest_unwrap,

            /*
             * Tuples
             */
            // future<tuple<R1, R2, ...>> -> f(R1, R2, ...)
            // future<tuple<future<R1>, future<R2>, ...>> -> f(future<R1>,
            // future<R2>, ...)
            is_valid<tuple_explode_unwrap>,
            tuple_explode_unwrap,

            // future<tuple<future<R1>, future<R2>, ...>> -> f(R1, R2, ...)
            // future<tuple<future<future<R1>>, future<future<R2>>, ...>> ->
            // f(future<R1>, future<R2>, ...)
            is_valid<futures_tuple_double_unwrap>,
            futures_tuple_double_unwrap,

            // future<tuple<future<R1>, future<R2>, ...>> -> f(R1, R2, ...)
            // future<tuple<future<future<R1>>, future<future<R2>>, ...>> ->
            // f(R1, R2, ...)
            is_valid<futures_tuple_deepest_unwrap>,
            futures_tuple_deepest_unwrap,

            /*
             * Ranges
             */
            // future<range<future<R>>> -> f(small_vector<R>)
            // future<range<future<future<R>>>> -> f(small_vector<future<R>>)
            is_valid<futures_range_double_unwrap>,
            futures_range_double_unwrap,

            // future<range<future<R>>> -> f(small_vector<R>)
            // future<range<future<future<R>>>> -> f(small_vector<R>)
            is_valid<futures_range_double_unwrap>,
            futures_range_deepest_unwrap,

            /*
             * When any disjunctions: tuples
             */
            // future<when_any_result<tuple<future<T1>, future<T2>, ...>> ->
            // f(size_t, tuple<future<T1>, future<T2>, ...>)
            is_valid<when_any_split_unwrap>,
            when_any_split_unwrap,

            // future<when_any_result<tuple<future<T1>, future<T2>, ...>> ->
            // f(size_t, future<T1>, future<T2>, ...)
            is_valid<when_any_explode_unwrap>,
            when_any_explode_unwrap,

            // future<when_any_result<tuple<future<R>, ...>> -> f(future<R>)
            is_valid<when_any_tuple_element_unwrap>,
            when_any_tuple_element_unwrap,

            // future<when_any_result<range<future<R>, ...>> -> f(future<R>)
            is_valid<when_any_range_element_unwrap>,
            when_any_range_element_unwrap,

            // future<when_any_result<tuple<future<R>, ...>> -> f(R)
            // future<when_any_result<tuple<future<future<R>>, ...>> ->
            // f(future<R>)
            is_valid<when_any_tuple_double_unwrap>,
            when_any_tuple_double_unwrap,

            // future<when_any_result<tuple<future<R>, ...>> -> f(R)
            // future<when_any_result<tuple<future<future<R>>, ...>> -> f(R)
            is_valid<when_any_tuple_deepest_unwrap>,
            when_any_tuple_deepest_unwrap,

            // future<when_any_result<range<future<R>, ...>> -> f(R)
            // future<when_any_result<range<future<future<R>>, ...>> ->
            // f(future<R>)
            is_valid<when_any_range_double_unwrap>,
            when_any_range_double_unwrap,

            // future<when_any_result<range<future<R>, ...>> -> f(R)
            // future<when_any_result<range<future<future<R>>, ...>> -> f(R)
            is_valid<when_any_range_deepest_unwrap>,
            when_any_range_deepest_unwrap,

            // failure
            std::true_type,
            failure>;
    };

    template <class Future, class Function, class... PrefixArgs>
    using continue_tag_t =
        typename continue_tag<Future, Function, PrefixArgs...>::type;

    // --------------------------------------------------------------
    // continue result types
    //
    // find the continuation result type given the continuation tag and
    // the specified antecedent future
    //

    template <class Tag, class Future, class Function, class... PrefixArgs>
    using continue_invoke_result_for = typename continue_invoke_traits<
        Tag,
        Future,
        mp_list<PrefixArgs...>,
        Function>::result;

    template <class Tag, class Future, class Function, class... PrefixArgs>
    using continue_invoke_result_for_t = typename continue_invoke_result_for<
        Tag,
        Future,
        Function,
        PrefixArgs...>::type;

    template <class Future, class Function, class... PrefixArgs>
    using continue_invoke_result = continue_invoke_result_for<
        continue_tag_t<Future, Function, PrefixArgs...>,
        Future,
        Function,
        PrefixArgs...>;

    template <class Future, class Function, class... PrefixArgs>
    using continue_invoke_result_t =
        typename continue_invoke_result<Future, Function, PrefixArgs...>::type;

    template <class Tag, class Future, class Function, class... PrefixArgs>
    using continue_is_invocable_for = typename continue_invoke_traits<
        Tag,
        Future,
        mp_list<PrefixArgs...>,
        Function>::valid;

    template <class Tag, class Future, class Function, class... PrefixArgs>
    static constexpr bool continue_is_invocable_for_v
        = continue_is_invocable_for<Tag, Future, Function, PrefixArgs...>::value;

    template <class Future, class Function, class... PrefixArgs>
    using continue_is_invocable = continue_is_invocable_for<
        continue_tag_t<Future, Function, PrefixArgs...>,
        Future,
        Function,
        PrefixArgs...>;

    template <class Future, class Function, class... PrefixArgs>
    static constexpr bool continue_is_invocable_v
        = continue_is_invocable<Future, Function, PrefixArgs...>::value;

} // namespace futures::detail

#endif // FUTURES_ADAPTOR_DETAIL_CONTINUE_INVOKE_TAG_HPP
