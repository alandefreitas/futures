//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TUPLE_TYPE_ALL_OF_H
#define FUTURES_TUPLE_TYPE_ALL_OF_H

#include <tuple>
#include <type_traits>

#include <futures/adaptor/detail/traits/is_tuple.h>

namespace futures::detail {
    /// \brief Check if all types in a tuple match a predicate
    template <class T, template <class...> class P> struct tuple_type_all_of : is_tuple<T> {};

    template <class T1, template <class...> class P> struct tuple_type_all_of<std::tuple<T1>, P> : P<T1> {};

    template <class T1, class... Tn, template <class...> class P>
    struct tuple_type_all_of<std::tuple<T1, Tn...>, P>
        : std::bool_constant<P<T1>::value && tuple_type_all_of<std::tuple<Tn...>, P>::value> {};

    template <class L, template <class...> class P>
    constexpr bool tuple_type_all_of_v = tuple_type_all_of<L, P>::value;

} // namespace futures::detail

#endif // FUTURES_TUPLE_TYPE_ALL_OF_H
