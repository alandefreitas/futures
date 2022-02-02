//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_TRAITS_IS_TYPE_TEMPLATE_IN_ARGS_HPP
#define FUTURES_FUTURES_DETAIL_TRAITS_IS_TYPE_TEMPLATE_IN_ARGS_HPP

#include <futures/futures/detail/traits/type_member_or_void.hpp>
#include <type_traits>

namespace futures::detail {
    template <template <class> class T, class... Args>
    struct is_type_template_in_args : std::false_type
    {};

    template <template <class> class T, class Arg>
    struct is_type_template_in_args<T, Arg>
        : std::is_same<T<type_member_or_void_t<Arg>>, Arg>
    {};

    template <template <class> class T, class Arg1, class... Args>
    struct is_type_template_in_args<T, Arg1, Args...>
        : std::conditional_t<
              std::is_same_v<T<type_member_or_void_t<Arg1>>, Arg1>,
              std::true_type,
              is_type_template_in_args<T, Args...>>
    {};

    template <template <class> class T, class... Args>
    inline constexpr bool is_type_template_in_args_v
        = is_type_template_in_args<T, Args...>::value;
} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_TRAITS_IS_TYPE_TEMPLATE_IN_ARGS_HPP
