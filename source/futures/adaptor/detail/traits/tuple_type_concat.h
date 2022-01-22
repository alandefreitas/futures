//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TUPLE_TYPE_CONCAT_H
#define FUTURES_TUPLE_TYPE_CONCAT_H

#include <tuple>

namespace futures::detail {
    /// \brief Concatenate type lists
    /// The detail functions related to type lists assume we use std::tuple for
    /// all type lists
    template <class...>
    struct tuple_type_concat
    {
        using type = std::tuple<>;
    };

    template <class T1>
    struct tuple_type_concat<T1>
    {
        using type = T1;
    };

    template <class... First, class... Second>
    struct tuple_type_concat<std::tuple<First...>, std::tuple<Second...>>
    {
        using type = std::tuple<First..., Second...>;
    };

    template <class T1, class... Tn>
    struct tuple_type_concat<T1, Tn...>
    {
        using type = typename tuple_type_concat<
            T1,
            typename tuple_type_concat<Tn...>::type>::type;
    };

    template <class... Tn>
    using tuple_type_concat_t = typename tuple_type_concat<Tn...>::type;
} // namespace futures::detail

#endif // FUTURES_TUPLE_TYPE_CONCAT_H
