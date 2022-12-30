//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_IMPL_WHEN_ANY_HPP
#define FUTURES_ADAPTOR_IMPL_WHEN_ANY_HPP

namespace futures {
    template <class T>
    struct is_future<when_any_future<T>> : std::true_type {};

    namespace detail {
        // Check if type is a when_any_future as a type
        template <class>
        struct is_when_any_future : std::false_type {};
        template <class Sequence>
        struct is_when_any_future<when_any_future<Sequence>>
            : std::true_type {};

        // Check if type is a when_any_future as constant bool
        template <class T>
        constexpr bool is_when_any_future_v = is_when_any_future<T>::value;

        // Check if a type can be used in a future disjunction (when_any
        // or operator|| for futures)
        template <class T>
        using is_valid_when_any_argument
            = disjunction<is_future<std::decay_t<T>>, detail::is_invocable<T>>;

        template <class T>
        constexpr bool is_valid_when_any_argument_v
            = is_valid_when_any_argument<T>::value;

        // Trait to identify valid when_any inputs
        template <class...>
        struct are_valid_when_any_arguments : std::true_type {};
        template <class B1>
        struct are_valid_when_any_arguments<B1>
            : is_valid_when_any_argument<B1> {};
        template <class B1, class... Bn>
        struct are_valid_when_any_arguments<B1, Bn...>
            : std::conditional_t<
                  is_valid_when_any_argument_v<B1>,
                  are_valid_when_any_arguments<Bn...>,
                  std::false_type> {};
        template <class... Args>
        constexpr bool are_valid_when_any_arguments_v
            = are_valid_when_any_arguments<Args...>::value;

        // @name Helpers for operator|| on futures, functions and
        // when_any futures

        // Check if type is a when_any_future with tuples as a sequence
        // type
        template <class T, class Enable = void>
        struct is_when_any_tuple_future : std::false_type {};
        template <class Sequence>
        struct is_when_any_tuple_future<
            when_any_future<Sequence>,
            std::enable_if_t<detail::mp_similar<std::tuple<>, Sequence>::value>>
            : std::true_type {};
        template <class T>
        constexpr bool is_when_any_tuple_future_v = is_when_any_tuple_future<
            T>::value;

        // Check if all template parameters are when_any_future with
        // tuples as a sequence type
        template <class...>
        struct are_when_any_tuple_futures : std::true_type {};
        template <class B1>
        struct are_when_any_tuple_futures<B1>
            : is_when_any_tuple_future<std::decay_t<B1>> {};
        template <class B1, class... Bn>
        struct are_when_any_tuple_futures<B1, Bn...>
            : std::conditional_t<
                  is_when_any_tuple_future_v<std::decay_t<B1>>,
                  are_when_any_tuple_futures<Bn...>,
                  std::false_type> {};
        template <class... Args>
        constexpr bool are_when_any_tuple_futures_v
            = are_when_any_tuple_futures<Args...>::value;

        // Check if type is a when_any_future with a range as a sequence
        // type
        template <class T, class Enable = void>
        struct is_when_any_range_future : std::false_type {};
        template <class Sequence>
        struct is_when_any_range_future<
            when_any_future<Sequence>,
            std::enable_if_t<is_range_v<Sequence>>> : std::true_type {};
        template <class T>
        constexpr bool is_when_any_range_future_v = is_when_any_range_future<
            T>::value;

        // Check if all template parameters are when_any_future with
        // tuples as a sequence type
        template <class...>
        struct are_when_any_range_futures : std::true_type {};
        template <class B1>
        struct are_when_any_range_futures<B1> : is_when_any_range_future<B1> {};
        template <class B1, class... Bn>
        struct are_when_any_range_futures<B1, Bn...>
            : std::conditional_t<
                  is_when_any_range_future_v<B1>,
                  are_when_any_range_futures<Bn...>,
                  std::false_type> {};
        template <class... Args>
        constexpr bool are_when_any_range_futures_v
            = are_when_any_range_futures<Args...>::value;


        struct move_share_or_post_fn {
            template <class F>
            auto
            operator()(F &&f) {
                return impl(
                    mp_cond<
                        is_shared_future<std::decay_t<F>>,
                        mp_int<0>,
                        is_future<std::decay_t<F>>,
                        mp_int<1>,
                        std::true_type /* is_invocable<F>*/,
                        mp_int<2>>{},
                    std::forward<F>(f));
            }

        private:
            // shared future
            template <class F>
            auto
            impl(mp_int<0>, F &&f) {
                return std::forward<decltype(f)>(f);
            }

            // unique future
            template <class F>
            auto
            impl(mp_int<1>, F &&f) {
                return std::move(std::forward<decltype(f)>(f));
            }

            // callable
            template <class F>
            auto
            impl(mp_int<2>, F &&f) {
                return ::futures::async(std::forward<decltype(f)>(f));
            }
        };

        struct maybe_make_disjunction_future_fn {
            template <class F>
            auto
            operator()(F &&f) {
                return impl(
                    mp_cond<
                        conjunction<is_invocable<F>, mp_not<is_future<F>>>,
                        mp_int<0>,
                        is_shared_future<F>,
                        mp_int<1>,
                        std::true_type,
                        mp_int<2>>{},
                    std::forward<F>(f));
            }

        private:
            template <class F>
            auto
            impl(mp_int<0>, F &&f) {
                return async(std::forward<F>(f));
            }

            template <class F>
            auto
            impl(mp_int<1>, F &&f) {
                return std::forward<F>(f);
            }

            template <class F>
            auto
            impl(mp_int<2>, F &&f) {
                return std::move(std::forward<F>(f));
            }
        };

        // Constructs a when_any_future that is a concatenation of all
        // when_any_futures in args It's important to be able to merge
        // when_any_future objects because of operator|| When the user asks for
        // f1 && f2 && f3, we want that to return a single future that waits
        // for <f1,f2,f3> rather than a future that wait for two futures
        // <f1,<f2,f3>> @note This function only participates in overload
        // resolution if all types in std::decay_t<WhenAllFutures>... are
        // specializations of when_any_future with a tuple sequence type
        //
        // @note "Merging" a single when_any_future of tuples. Overload
        // provided for symmetry.
        //
        template <
            class WhenAllFuture,
            std::enable_if_t<is_when_any_tuple_future_v<WhenAllFuture>, int> = 0>
        FUTURES_DETAIL(decltype(auto))
        when_any_future_cat(WhenAllFuture &&arg0) {
            return std::forward<WhenAllFuture>(arg0);
        }

        // Overload merging a two when_any_future objects of tuples
        template <
            class WhenAllFuture1,
            class WhenAllFuture2,
            std::enable_if_t<
                are_when_any_tuple_futures_v<WhenAllFuture1, WhenAllFuture2>,
                int>
            = 0>
        FUTURES_DETAIL(decltype(auto))
        when_any_future_cat(WhenAllFuture1 &&arg0, WhenAllFuture2 &&arg1) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(std::forward<WhenAllFuture2>(arg1).release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_any_future<decltype(s)>(std::move(s));
        }

        // Overload merging two+ when_any_future of tuples
        template <
            class WhenAllFuture1,
            class... WhenAllFutures,
            std::enable_if_t<
                are_when_any_tuple_futures_v<WhenAllFuture1, WhenAllFutures...>,
                int>
            = 0>
        FUTURES_DETAIL(decltype(auto))
        when_any_future_cat(WhenAllFuture1 &&arg0, WhenAllFutures &&...args) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(
                when_any_future_cat(std::forward<WhenAllFutures>(args)...)
                    .release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_any_future<decltype(s)>(std::move(s));
        }

        FUTURES_TEMPLATE(class T1, class T2)
        (requires detail::is_valid_when_any_argument_v<T1> &&detail::
             is_valid_when_any_argument_v<T2> &&detail::is_when_any_future_v<T1>
                 &&detail::is_when_any_future_v<T2>) FUTURES_DETAIL(auto)
            disjunction_impl(T1 &&lhs, T2 &&rhs) {
            return detail::when_any_future_cat(
                std::forward<T1>(lhs),
                std::forward<T2>(rhs));
        }

        FUTURES_TEMPLATE(class T1, class T2)
        (requires detail::is_valid_when_any_argument_v<T1>
             &&detail::is_valid_when_any_argument_v<T2>
         && (!detail::is_when_any_future_v<T1>) &&(
             !detail::is_when_any_future_v<T2>) ) FUTURES_DETAIL(auto)
            disjunction_impl(T1 &&lhs, T2 &&rhs) {
            return when_any(
                maybe_make_disjunction_future_fn{}(std::forward<T1>(lhs)),
                maybe_make_disjunction_future_fn{}(std::forward<T2>(rhs)));
        }

        FUTURES_TEMPLATE(class T1, class T2)
        (requires detail::is_valid_when_any_argument_v<T1>
             &&detail::is_valid_when_any_argument_v<T2>
                 &&detail::is_when_any_future_v<T1>
         && (!detail::is_when_any_future_v<T2>) ) FUTURES_DETAIL(auto)
            disjunction_impl(T1 &&lhs, T2 &&rhs) {
            // If one of them is a when_any_future, then we need to
            // concatenate the results rather than creating a child in the
            // sequence. To concatenate them, the one that is not a
            // when_any_future needs to become one.
            return detail::when_any_future_cat(
                lhs,
                when_any(
                    maybe_make_disjunction_future_fn{}(std::forward<T2>(rhs))));
        }

        FUTURES_TEMPLATE(class T1, class T2)
        (requires detail::is_valid_when_any_argument_v<T1>
             &&detail::is_valid_when_any_argument_v<T2>
         && (!detail::is_when_any_future_v<T1>) &&detail::is_when_any_future_v<
             T2>) FUTURES_DETAIL(auto) disjunction_impl(T1 &&lhs, T2 &&rhs) {
            return detail::when_any_future_cat(
                when_any(
                    maybe_make_disjunction_future_fn{}(std::forward<T1>(lhs))),
                rhs);
        }

        template <class Range, class InputIt>
        void
        range_push_back_impl2(mp_int<0>, Range &v, InputIt first, InputIt last) {
            std::copy(first, last, std::back_inserter(v));
        }

        template <class Range, class InputIt>
        void
        range_push_back_impl2(mp_int<1>, Range &v, InputIt first, InputIt last) {
            std::move(first, last, std::back_inserter(v));
        }

        template <class Range, class InputIt>
        void
        range_push_back_impl2(mp_int<2>, Range &v, InputIt first, InputIt last) {
            FUTURES_STATIC_ASSERT(
                detail::is_invocable_v<std::decay_t<
                    typename std::iterator_traits<InputIt>::value_type>>);
            std::transform(first, last, std::back_inserter(v), [](auto &&f) {
                return std::move(futures::async(std::forward<decltype(f)>(f)));
            });
        }
    } // namespace detail

#ifdef FUTURES_HAS_CONCEPTS
    template <class InputIt>
    requires(
        is_future_v<
            std::decay_t<typename std::iterator_traits<InputIt>::value_type>>
        || detail::is_invocable_v<
            typename std::iterator_traits<InputIt>::value_type>)
#else
    template <
        class InputIt,
        std::enable_if_t<
            detail::disjunction_v<
                is_future<
                    std::decay_t<typename std::iterator_traits<InputIt>::value_type>>,
                detail::is_invocable<
                    typename std::iterator_traits<InputIt>::value_type>>,
            int>>
#endif
    when_any_future<
        FUTURES_DETAIL(detail::small_vector<detail::lambda_to_future_t<
                           typename std::iterator_traits<InputIt>::
                               value_type>>)> when_any(InputIt first, InputIt last) {
        // Infer types
        using input_type = std::decay_t<
            typename std::iterator_traits<InputIt>::value_type>;
        constexpr bool input_is_future = is_future_v<input_type>;
        constexpr bool input_is_invocable = detail::is_invocable_v<input_type>;
        FUTURES_STATIC_ASSERT(input_is_future || input_is_invocable);
        using output_future_type = detail::lambda_to_future_t<input_type>;
        using sequence_type = detail::small_vector<output_future_type>;

        // Create sequence
        sequence_type v;
        v.reserve(std::distance(first, last));

        // Move or copy the future objects
        detail::range_push_back_impl2(
            boost::mp11::mp_cond<
                detail::conjunction<
                    is_future<input_type>,
                    is_shared_future<output_future_type>>,
                boost::mp11::mp_int<0>,
                detail::conjunction<
                    is_future<input_type>,
                    boost::mp11::mp_not<is_shared_future<output_future_type>>>,
                boost::mp11::mp_int<1>,
                std::true_type,
                boost::mp11::mp_int<2>>{},
            v,
            first,
            last);

        return when_any_future<sequence_type>(std::move(v));
    }

#ifdef FUTURES_HAS_CONCEPTS
    template <class... Futures>
    requires detail::conjunction_v<detail::disjunction<
        is_future<std::decay_t<Futures>>,
        detail::is_invocable<std::decay_t<Futures>>>...>
#else
    template <
        class... Futures,
        std::enable_if_t<
            detail::conjunction_v<detail::disjunction<
                is_future<std::decay_t<Futures>>,
                detail::is_invocable<std::decay_t<Futures>>>...>,
            int>>
#endif
    when_any_future<
        std::tuple<FUTURES_DETAIL(detail::lambda_to_future_t<Futures>...)>>
    when_any(Futures &&...futures) {
        // Infer sequence type
        using sequence_type = std::tuple<detail::lambda_to_future_t<Futures>...>;

        // Create sequence (and infer types as we go)
        sequence_type v = std::make_tuple(
            (detail::move_share_or_post_fn{}(futures))...);

        return when_any_future<sequence_type>(std::move(v));
    }

#ifdef FUTURES_HAS_CONCEPTS
    template <class T1, class T2>
    requires detail::disjunction_v<
                 is_future<std::decay_t<T1>>,
                 detail::is_invocable<std::decay_t<T1>>>
             && detail::disjunction_v<
                 is_future<std::decay_t<T2>>,
                 detail::is_invocable<std::decay_t<T2>>>
#else
    template <
        class T1,
        class T2,
        std::enable_if_t<
            detail::disjunction_v<
                is_future<std::decay_t<T1>>,
                detail::is_invocable<std::decay_t<T1>>>
                && detail::disjunction_v<
                    is_future<std::decay_t<T2>>,
                    detail::is_invocable<std::decay_t<T2>>>,
            int>>
#endif
    FUTURES_DETAIL(auto)
    operator||(T1 &&lhs, T2 &&rhs) {
        return detail::
            disjunction_impl(std::forward<T1>(lhs), std::forward<T2>(rhs));
    }

} // namespace futures
#endif // FUTURES_ADAPTOR_IMPL_WHEN_ANY_HPP
