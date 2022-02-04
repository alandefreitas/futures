//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_TUPLE_TYPE_ALL_OF_HPP
#define FUTURES_DETAIL_TRAITS_TUPLE_TYPE_ALL_OF_HPP

#include <futures/detail/traits/is_tuple.hpp>
#include <tuple>
#include <type_traits>

namespace futures::detail {
    /// \brief Check if all types in a tuple match a predicate
    template <class T, template <class...> class P>
    struct tuple_type_all_of : is_tuple<T>
    {};

    template <class T1, template <class...> class P>
    struct tuple_type_all_of<std::tuple<T1>, P> : P<T1>
    {};

    template <class T1, class... Tn, template <class...> class P>
    struct tuple_type_all_of<std::tuple<T1, Tn...>, P>
        : std::bool_constant<
              P<T1>::value && tuple_type_all_of<std::tuple<Tn...>, P>::value>
    {};

    template <class L, template <class...> class P>
    constexpr bool tuple_type_all_of_v = tuple_type_all_of<L, P>::value;

} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_TUPLE_TYPE_ALL_OF_HPP
