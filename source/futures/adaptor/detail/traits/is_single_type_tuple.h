//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TUPLE_TYPE_IS_SINGLE_TYPE_TUPLE_H
#define FUTURES_TUPLE_TYPE_IS_SINGLE_TYPE_TUPLE_H

#include <futures/adaptor/detail/traits/is_tuple.h>
#include <tuple>
#include <type_traits>

namespace futures::detail {
    /// \brief Check if all types in a tuple match a predicate
    template <class L> struct is_single_type_tuple : is_tuple<L> {};

    template <class T1> struct is_single_type_tuple<std::tuple<T1>> : std::true_type {};

    template <class T1, class T2> struct is_single_type_tuple<std::tuple<T1, T2>> : std::is_same<T1, T2> {};

    template <class T1, class T2, class... Tn>
    struct is_single_type_tuple<std::tuple<T1, T2, Tn...>>
        : std::bool_constant<std::is_same_v<T1, T2> && is_single_type_tuple<std::tuple<T2, Tn...>>::value> {};

    template <class L> constexpr bool is_single_type_tuple_v = is_single_type_tuple<L>::value;

} // namespace futures::detail

#endif // FUTURES_TUPLE_TYPE_IS_SINGLE_TYPE_TUPLE_H
