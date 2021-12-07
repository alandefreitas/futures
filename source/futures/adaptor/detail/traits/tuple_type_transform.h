//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TUPLE_TYPE_TRANSFORM_H
#define FUTURES_TUPLE_TYPE_TRANSFORM_H

#include <tuple>
#include <type_traits>

namespace futures::detail {
    /// \brief Transform all types in a tuple
    template <class L, template <class...> class P> struct tuple_type_transform {
        using type = std::tuple<>;
    };

    template <class... Tn, template <class...> class P>
    struct tuple_type_transform<std::tuple<Tn...>, P> {
        using type = std::tuple<typename P<Tn>::type...>;
    };

    template <class L, template <class...> class P>
    using tuple_type_transform_t = typename tuple_type_transform<L, P>::type;

} // namespace futures::detail

#endif // FUTURES_TUPLE_TYPE_TRANSFORM_H
