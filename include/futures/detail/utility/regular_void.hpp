//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_UTILITY_REGULAR_VOID_HPP
#define FUTURES_DETAIL_UTILITY_REGULAR_VOID_HPP

#include <futures/config.hpp>
#include <futures/detail/traits/is_reference_wrapper.hpp>
#include <futures/detail/traits/unwrap_refwrapper.hpp>
#include <futures/detail/deps/boost/mp11/bind.hpp>
#include <futures/detail/deps/boost/mp11/integer_sequence.hpp>
#include <futures/detail/deps/boost/mp11/utility.hpp>
#include <functional>
#include <tuple>
#include <utility>

namespace futures::detail {
    // A regular type that represents `void`
    // see: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0146r1.html
    struct regular_void {};

    // Convert type to regular void if irregular void
    template <class T>
    using make_regular = mp_defer<mp_if, std::is_void<T>, regular_void, T>;

    template <class T>
    using make_regular_t = typename make_regular<T>::type;

    // Convert any type to irregular void if regular void
    template <class T>
    T
    make_irregular(T&& x) {
        return std::forward<T>(x);
    }

    inline void
    make_irregular(regular_void) {}

    // an integer sequence as types
    template <class T>
    struct integer_type_sequence {};

    template <class T, T... Ints>
    struct integer_type_sequence<std::integer_sequence<T, Ints...>> {
        using type = mp_list<std::integral_constant<T, Ints>...>;
    };

    template <class T>
    using is_regular_void = mp_bind_front<std::is_same, regular_void>::fn<
        std::decay_t<T>>;

    // determine if first element is regular void
    template <class L>
    using front_is_regular_void = is_regular_void<mp_front<L>>;

    // tuple invoke result when we remove regular void from the args
    template <class F, class Tuple>
    using regular_tuple_invoke_result = make_regular<mp_apply<
        std::invoke_result_t,
        mp_append<mp_list<F>, mp_remove_if<Tuple, is_regular_void>>>>;

    template <class F, class Tuple>
    using regular_tuple_invoke_result_t =
        typename regular_tuple_invoke_result<F, Tuple>::type;


    // Invoke the function f with the elements Is of a tuple where Is is a list
    // of mp_size_t
    template <class F, class Tuple, class... Is>
    constexpr regular_tuple_invoke_result_t<F, Tuple>
    regular_tuple_invoke(F&& f, Tuple&& t, mp_list<Is...>) {
        if constexpr (is_regular_void<
                          regular_tuple_invoke_result_t<F, Tuple>>::value)
        {
            std::invoke(
                std::forward<F>(f),
                std::get<Is::value>(std::forward<Tuple>(t))...);
            return regular_void{};
        } else {
            return std::invoke(
                std::forward<F>(f),
                std::get<Is::value>(std::forward<Tuple>(t))...);
        }
    }

    // invoke result when we remove regular void from the args
    template <class F, class... Args>
    using regular_invoke_result = make_regular<mp_apply<
        std::invoke_result_t,
        mp_append<mp_list<F>, mp_remove_if<mp_list<Args...>, is_regular_void>>>>;

    template <class F, class... Args>
    using regular_invoke_result_t = typename regular_invoke_result<F, Args...>::
        type;

    template <class F, class... Args>
    using regular_is_no_throw_invocable = mp_apply<
        std::is_nothrow_invocable,
        mp_append<mp_list<F>, mp_remove_if<mp_list<Args...>, is_regular_void>>>;

    // Generate mp_list of mp_size_t of the indices that are not regular_void
    template <class... Args>
    using index_sequence_type_non_void = mp_transform<
        mp_second,
        mp_remove_if<
            mp_transform<
                mp_list,
                mp_list<Args...>,
                typename integer_type_sequence<
                    std::index_sequence_for<Args...>>::type>,
            front_is_regular_void>>;

    // Invoke the function removing any regular_void from args and returning
    // regular void if return type is void
    template <class F, class... Args>
    constexpr regular_invoke_result_t<F, Args...>
    regular_void_invoke(F&& f, Args&&... args) noexcept(
        regular_is_no_throw_invocable<F, Args...>::value) {
        return regular_tuple_invoke(
            std::forward<F>(f),
            std::forward_as_tuple(std::forward<Args>(args)...),
            index_sequence_type_non_void<Args...>{});
    }

    // result of make_irregular_tuple, which removes regular_void from the list
    template <class... Args>
    using make_irregular_tuple_result = mp_identity<mp_apply<
        std::tuple,
        mp_transform<
            unwrap_decay_t,
            mp_remove_if<mp_list<Args...>, is_regular_void>>>>;

    template <class... Args>
    using make_irregular_tuple_result_t = typename make_irregular_tuple_result<
        Args...>::type;

    template <class T>
    struct make_irregular_tuple_impl_result {};

    template <class... Args>
    struct make_irregular_tuple_impl_result<std::tuple<Args...>>
        : mp_defer<make_irregular_tuple_result_t, Args...> {};

    template <class T>
    using make_irregular_tuple_impl_result_t =
        typename make_irregular_tuple_impl_result<T>::type;

    // Invoke the function removing any regular_void from args and
    // returning
    // regular void if return type is void
    template <class... Is, class Tuple>
    constexpr make_irregular_tuple_impl_result_t<Tuple>
    make_irregular_tuple_impl(mp_list<Is...>, Tuple&& t) {
        return make_irregular_tuple_impl_result_t<Tuple>(
            std::get<Is::value>(std::forward<Tuple>(t))...);
    }

    template <class... Args>
    constexpr make_irregular_tuple_result_t<Args...>
    make_irregular_tuple(Args&&... args) {
        return make_irregular_tuple_impl(
            index_sequence_type_non_void<Args...>{},
            std::forward_as_tuple(std::forward<Args>(args)...));
    }

} // namespace futures::detail

#endif // FUTURES_DETAIL_UTILITY_REGULAR_VOID_HPP
