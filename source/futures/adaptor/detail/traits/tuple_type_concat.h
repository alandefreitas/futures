//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TUPLE_TYPE_CONCAT_H
#define FUTURES_TUPLE_TYPE_CONCAT_H

#include <tuple>

namespace futures::detail {
    /// \brief Concatenate type lists
    /// The detail functions related to type lists assume we use std::tuple for all type lists
    template<class...> struct tuple_type_concat {
        using type = std::tuple<>;
    };

    template<class T1> struct tuple_type_concat<T1> {
        using type = T1;
    };

    template <class... First, class... Second>
    struct tuple_type_concat<std::tuple<First...>, std::tuple<Second...>> {
        using type = std::tuple<First..., Second...>;
    };

    template<class T1, class... Tn>
    struct tuple_type_concat<T1, Tn...> {
        using type = typename tuple_type_concat<T1, typename tuple_type_concat<Tn...>::type>::type;
    };

    template<class... Tn>
    using tuple_type_concat_t = typename tuple_type_concat<Tn...>::type;
}

#endif // FUTURES_TUPLE_TYPE_CONCAT_H
