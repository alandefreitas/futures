//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_TUPLE_ALGORITHM_H
#define FUTURES_TUPLE_ALGORITHM_H

#include <small/detail/exception/throw.h>
#include <tuple>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    namespace detail {
        template<class Function, class... Args, std::size_t... Is>
        static void for_each_impl(const std::tuple<Args...> &t, Function &&fn, std::index_sequence<Is...>) {
            (fn(std::get<Is>(t)), ...);
        }
    } // namespace detail

    /// \brief tuple_for_each for tuples
    template<class Function, class... Args>
    static void tuple_for_each(const std::tuple<Args...> &t, Function &&fn) {
        detail::for_each_impl(t, std::forward<Function>(fn), std::index_sequence_for<Args...>{});
    }

    namespace detail {
        template<class Function, class... Args1, class... Args2, std::size_t... Is>
        static void for_each_paired_impl(std::tuple<Args1...> &t1, std::tuple<Args2...> &t2, Function &&fn,
                                         std::index_sequence<Is...>) {
            (fn(std::get<Is>(t1), std::get<Is>(t2)), ...);
        }
    } // namespace detail

    /// \brief for_each_paired for paired tuples of same size
    template<class Function, class... Args1, class... Args2>
    static void for_each_paired(std::tuple<Args1...> &t1, std::tuple<Args2...> &t2, Function &&fn) {
        static_assert(std::tuple_size_v<std::tuple<Args1...>> == std::tuple_size_v<std::tuple<Args2...>>);
        detail::for_each_paired_impl(t1, t2, std::forward<Function>(fn), std::index_sequence_for<Args1...>{});
    }

    namespace detail {
        template<class Function, class... Args1, class T, size_t N, std::size_t... Is>
        static void for_each_paired_impl(std::tuple<Args1...> &t, std::array<T, N> &a, Function &&fn,
                                         std::index_sequence<Is...>) {
            (fn(std::get<Is>(t), a[Is]), ...);
        }
    } // namespace detail

    /// \brief for_each_paired for paired tuples and arrays of same size
    template<class Function, class... Args1, class T, size_t N>
    static void for_each_paired(std::tuple<Args1...> &t, std::array<T, N> &a, Function &&fn) {
        static_assert(std::tuple_size_v<std::tuple<Args1...>> == N);
        detail::for_each_paired_impl(t, a, std::forward<Function>(fn), std::index_sequence_for<Args1...>{});
    }

    /// \brief find_if for tuples
    template<class Function, size_t t_idx = 0, class... Args>
    static size_t tuple_find_if(const std::tuple<Args...> &t, Function &&fn) {
        if constexpr (t_idx == std::tuple_size_v<std::decay_t<decltype(t)>>) {
            return t_idx;
        } else {
            if (fn(std::get<t_idx>(t))) {
                return t_idx;
            }
            return tuple_find_if<Function, t_idx + 1, Args...>(t, std::forward<Function>(fn));
        }
    }

    namespace detail {
        template<class Function, class... Args, std::size_t... Is>
        static bool all_of_impl(const std::tuple<Args...> &t, Function &&fn, std::index_sequence<Is...>) {
            return (fn(std::get<Is>(t)) && ...);
        }
    } // namespace detail

    /// \brief all_of for tuples
    template<class Function, class... Args>
    static bool tuple_all_of(const std::tuple<Args...> &t, Function &&fn) {
        return detail::all_of_impl(t, std::forward<Function>(fn), std::index_sequence_for<Args...>{});
    }

    namespace detail {
        template<class Function, class... Args, std::size_t... Is>
        static bool any_of_impl(const std::tuple<Args...> &t, Function &&fn, std::index_sequence<Is...>) {
            return (fn(std::get<Is>(t)) || ...);
        }
    } // namespace detail

    /// \brief any_of for tuples
    template<class Function, class... Args>
    static bool tuple_any_of(const std::tuple<Args...> &t, Function &&fn) {
        return detail::any_of_impl(t, std::forward<Function>(fn), std::index_sequence_for<Args...>{});
    }

    /// \brief Apply a function to a single tuple element at runtime
    /// The function must, of course, be valid for all tuple elements
    template<class Function, class Tuple, size_t current_tuple_idx = 0,
            std::enable_if_t<is_callable_v < Function> &&is_tuple_v<Tuple> &&
            (current_tuple_idx < std::tuple_size_v<std::decay_t<Tuple>>),
    int> = 0>

    constexpr static auto apply(Function &&fn, Tuple &&t, std::size_t idx) {
        assert(idx < std::tuple_size_v<std::decay_t<Tuple>>);
        if (current_tuple_idx == idx) {
            return fn(std::get<current_tuple_idx>(t));
        } else if constexpr (current_tuple_idx + 1 < std::tuple_size_v<std::decay_t<Tuple>>) {
            return apply < Function, Tuple, current_tuple_idx + 1 > (std::forward<Function>(fn), std::forward<Tuple>(t),
                    idx);
        } else {
            small::throw_exception<std::out_of_range>("apply:: tuple idx out of range");
        }
    }

    /// \brief Return the i-th element from a tuple whose types are the same
    /// The return expression function must, of course, be valid for all tuple elements
    template<
            class Tuple, size_t current_tuple_idx = 0,
            std::enable_if_t<
                    is_tuple_v < Tuple> &&(current_tuple_idx < std::tuple_size_v<std::decay_t<Tuple>>), int> = 0>

    constexpr static decltype(auto) get(Tuple &&t, std::size_t idx) {
        assert(idx < std::tuple_size_v<std::decay_t<Tuple>>);
        if (current_tuple_idx == idx) {
            return std::get<current_tuple_idx>(t);
        } else if constexpr (current_tuple_idx + 1 < std::tuple_size_v<std::decay_t<Tuple>>) {
            return get < Tuple, current_tuple_idx + 1 > (std::forward<Tuple>(t), idx);
        } else {
            small::throw_exception<std::out_of_range>("get:: tuple idx out of range");
        }
    }

    /// \brief Return the i-th element from a tuple with a transformation function whose return is always the same
    /// The return expression function must, of course, be valid for all tuple elements
    template<
            class Tuple, size_t current_tuple_idx = 0, class TransformFn,
            std::enable_if_t<
                    is_tuple_v < Tuple> &&(current_tuple_idx < std::tuple_size_v<std::decay_t<Tuple>>), int> = 0>

    constexpr static decltype(auto) get(Tuple &&t, std::size_t idx, TransformFn &&transform) {
        assert(idx < std::tuple_size_v<std::decay_t<Tuple>>);
        if (current_tuple_idx == idx) {
            return transform(std::get<current_tuple_idx>(t));
        } else if constexpr (current_tuple_idx + 1 < std::tuple_size_v<std::decay_t<Tuple>>) {
            return get < Tuple, current_tuple_idx + 1 > (std::forward<Tuple>(t), idx, transform);
        } else {
            small::throw_exception<std::out_of_range>("get:: tuple idx out of range");
        }
    }

    namespace detail {
        template<class F, class FT, class Tuple, std::size_t... I>
        constexpr decltype(auto) transform_and_apply_impl(F &&f, FT &&ft, Tuple &&t, std::index_sequence<I...>) {
            return std::invoke(std::forward<F>(f), ft(std::get<I>(std::forward<Tuple>(t)))...);
        }
    } // namespace detail

    template<class F, class FT, class Tuple>
    constexpr decltype(auto) transform_and_apply(F &&f, FT &&ft, Tuple &&t) {
        return detail::transform_and_apply_impl(
                std::forward<F>(f), std::forward<FT>(ft), std::forward<Tuple>(t),
                std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
    }

    namespace detail {
        /// The tuple type after we filtered it with a template template predicate
        template<template<typename> typename UnaryPredicate, typename Tuple>
        struct filtered_tuple_type;

        /// The tuple type after we filtered it with a template template predicate
        template<template<typename> typename UnaryPredicate, typename... Ts>
        struct filtered_tuple_type<UnaryPredicate, std::tuple<Ts...>> {
            /// If this element has to be kept, returns `std::tuple<Ts>`
            /// Otherwise returns `std::tuple<>`
            template<class E>
            using t_filtered_tuple_type_impl =
            std::conditional_t<UnaryPredicate<E>::value, std::tuple<E>, std::tuple<>>;

            /// Determines the type that would be returned by `std::tuple_cat`
            ///  if it were called with instances of the types reported by
            ///  t_filtered_tuple_type_impl for each element
            using type = decltype(std::tuple_cat(std::declval<t_filtered_tuple_type_impl<Ts>>()...));
        };

        /// The tuple type after we filtered it with a template template predicate
        template<template<typename> typename UnaryPredicate, typename Tuple>
        struct transformed_tuple;

        /// The tuple type after we filtered it with a template template predicate
        template<template<typename> typename UnaryPredicate, typename... Ts>
        struct transformed_tuple<UnaryPredicate, std::tuple<Ts...>> {
            /// If this element has to be kept, returns `std::tuple<Ts>`
            /// Otherwise returns `std::tuple<>`
            template<class E> using transformed_tuple_element_type = typename UnaryPredicate<E>::type;

            /// Determines the type that would be returned by `std::tuple_cat`
            ///  if it were called with instances of the types reported by
            ///  transformed_tuple_element_type for each element
            using type = decltype(std::tuple_cat(std::declval<transformed_tuple_element_type<Ts>>()...));
        };
    } // namespace detail

    /// \brief Filter tuple elements based on their types
    template<template<typename> typename UnaryPredicate, typename... Ts>
    constexpr typename detail::filtered_tuple_type<UnaryPredicate, std::tuple<Ts...>>::type
    filter_if(const std::tuple<Ts...> &tup) {
        return std::apply(
                [](auto... tuple_value) {
                    return std::tuple_cat(std::conditional_t<UnaryPredicate<decltype(tuple_value)>::value,
                            std::tuple<decltype(tuple_value)>, std::tuple<>>{}...);
                },
                tup);
    }

    /// \brief Remove tuple elements based on their types
    template<template<typename> typename UnaryPredicate, typename... Ts>
    constexpr typename detail::filtered_tuple_type<UnaryPredicate, std::tuple<Ts...>>::type
    remove_if(const std::tuple<Ts...> &tup) {
        return std::apply(
                [](auto... tuple_value) {
                    return std::tuple_cat(std::conditional_t<not UnaryPredicate<decltype(tuple_value)>::value,
                            std::tuple<decltype(tuple_value)>, std::tuple<>>{}...);
                },
                tup);
    }

    /// \brief Transform tuple elements based on their types
    template<template<typename> typename UnaryPredicate, typename... Ts>
    constexpr typename detail::transformed_tuple<UnaryPredicate, std::tuple<Ts...>>::type
    transform(const std::tuple<Ts...> &tup) {
        return std::apply(
                [](auto... tuple_value) {
                    return std::tuple_cat(
                            std::tuple<typename UnaryPredicate<decltype(tuple_value)>::type>{tuple_value}...);
                },
                tup);
    }

    /** @} */  // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TUPLE_ALGORITHM_H
