//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_IS_SINGLE_TYPE_TUPLE_HPP
#define FUTURES_DETAIL_TRAITS_IS_SINGLE_TYPE_TUPLE_HPP

#include <futures/detail/traits/is_tuple.hpp>
#include <tuple>
#include <type_traits>

namespace futures::detail {
    /// Check if all types in a tuple match a predicate
    template <class L>
    struct is_single_type_tuple : is_tuple<L>
    {};

    template <class T1>
    struct is_single_type_tuple<std::tuple<T1>> : std::true_type
    {};

    template <class T1, class T2>
    struct is_single_type_tuple<std::tuple<T1, T2>> : std::is_same<T1, T2>
    {};

    template <class T1, class T2, class... Tn>
    struct is_single_type_tuple<std::tuple<T1, T2, Tn...>>
        : std::bool_constant<
              std::is_same_v<
                  T1,
                  T2> && is_single_type_tuple<std::tuple<T2, Tn...>>::value>
    {};

    template <class L>
    constexpr bool is_single_type_tuple_v = is_single_type_tuple<L>::value;

} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_IS_SINGLE_TYPE_TUPLE_HPP
