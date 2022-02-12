//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_TUPLE_TYPE_TRANSFORM_HPP
#define FUTURES_DETAIL_TRAITS_TUPLE_TYPE_TRANSFORM_HPP

#include <tuple>
#include <type_traits>

namespace futures::detail {
    /// Transform all types in a tuple
    template <class L, template <class...> class P>
    struct tuple_type_transform
    {
        using type = std::tuple<>;
    };

    template <class... Tn, template <class...> class P>
    struct tuple_type_transform<std::tuple<Tn...>, P>
    {
        using type = std::tuple<typename P<Tn>::type...>;
    };

    template <class L, template <class...> class P>
    using tuple_type_transform_t = typename tuple_type_transform<L, P>::type;

} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_TUPLE_TYPE_TRANSFORM_HPP
