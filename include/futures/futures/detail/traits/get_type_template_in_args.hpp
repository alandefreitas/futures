//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_TRAITS_GET_TYPE_TEMPLATE_IN_ARGS_HPP
#define FUTURES_FUTURES_DETAIL_TRAITS_GET_TYPE_TEMPLATE_IN_ARGS_HPP

#include <futures/futures/detail/traits/has_same_type_member.hpp>
#include <futures/detail/traits/type_member_or_void.hpp>
#include <type_traits>

namespace futures::detail {
    template <class Default, template <class> class T, class...>
    struct get_type_template_in_args
    {
        using type = Default;
    };

    template <class Default, template <class> class T, class Arg>
    struct get_type_template_in_args<Default, T, Arg>
    {
        using type = std::conditional_t<
            std::is_same_v<T<type_member_or_void_t<Arg>>, Arg>,
            type_member_or_void_t<Arg>,
            Default>;
    };

    template <class Default, template <class> class T, class Arg1, class... Args>
    struct get_type_template_in_args<Default, T, Arg1, Args...>
    {
        using type = std::conditional_t<
            std::is_same_v<T<type_member_or_void_t<Arg1>>, Arg1>,
            type_member_or_void_t<Arg1>,
            typename get_type_template_in_args<Default, T, Args...>::type>;
    };

    template <class Default, template <class> class T, class... Args>
    using get_type_template_in_args_t =
        typename get_type_template_in_args<Default, T, Args...>::type;

} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_TRAITS_GET_TYPE_TEMPLATE_IN_ARGS_HPP
