//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_IS_IN_ARGS_HPP
#define FUTURES_DETAIL_TRAITS_IS_IN_ARGS_HPP

#include <type_traits>

namespace futures::detail {
    template <class T, class...>
    struct is_in_args : std::false_type
    {};

    template <class T, class Arg>
    struct is_in_args<T, Arg> : std::is_same<T, Arg>
    {};

    template <class T, class Arg1, class... Args>
    struct is_in_args<T, Arg1, Args...>
        : std::conditional_t<
              std::is_same_v<T, Arg1>,
              std::true_type,
              is_in_args<T, Args...>>
    {};

    template <class T, class... Args>
    inline constexpr bool is_in_args_v = is_in_args<T, Args...>::value;
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_IS_IN_ARGS_HPP
