//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_INDEX_IN_ARGS_HPP
#define FUTURES_DETAIL_TRAITS_INDEX_IN_ARGS_HPP

#include <type_traits>

namespace futures::detail {
    template <std::size_t IDX_CANDIDATE, class T, class...>
    struct index_in_args_impl
        : std::integral_constant<std::size_t, std::size_t(-1)> {};

    template <std::size_t IDX_CANDIDATE, class T, class Arg>
    struct index_in_args_impl<IDX_CANDIDATE, T, Arg>
        : std::conditional_t<
              std::is_same_v<T, Arg>,
              std::integral_constant<std::size_t, IDX_CANDIDATE>,
              std::integral_constant<std::size_t, std::size_t(-1)>> {};

    template <std::size_t IDX_CANDIDATE, class T, class Arg1, class... Args>
    struct index_in_args_impl<IDX_CANDIDATE, T, Arg1, Args...>
        : std::conditional_t<
              std::is_same_v<T, Arg1>,
              std::integral_constant<std::size_t, IDX_CANDIDATE>,
              index_in_args_impl<IDX_CANDIDATE + 1, T, Args...>> {};

    template <class T, class... Args>
    struct index_in_args : index_in_args_impl<std::size_t(0), T, Args...> {};

    template <class T, class... Args>
    inline constexpr std::size_t index_in_args_v = index_in_args<T, Args...>::
        value;
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_INDEX_IN_ARGS_HPP
