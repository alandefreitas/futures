//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_TRAITS_REMOVE_FUTURE_OPTION_HPP
#define FUTURES_FUTURES_DETAIL_TRAITS_REMOVE_FUTURE_OPTION_HPP

#include <futures/futures/future_options.hpp>
#include <futures/futures/detail/traits/prepend_future_option.hpp>

namespace futures::detail {
    template <class Opt, class Opts>
    struct remove_future_option
    {
        using type = Opts;
    };

    template <class Opt>
    struct remove_future_option<Opt, future_options<>>
    {
        using type = future_options<>;
    };

    template <class Opt>
    struct remove_future_option<Opt, future_options<Opt>>
    {
        using type = future_options<>;
    };

    template <class Opt, class Arg0>
    struct remove_future_option<Opt, future_options<Arg0>>
    {
        using type = future_options<Arg0>;
    };

    template <class Opt, class Arg0, class... Args>
    struct remove_future_option<Opt, future_options<Arg0, Args...>>
    {
        using type = prepend_future_option_t<
            Arg0,
            typename remove_future_option<Opt, future_options<Args...>>::type>;
    };

    template <class Opt, class Opts>
    using remove_future_option_t = typename remove_future_option<Opt, Opts>::
        type;

} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_TRAITS_REMOVE_FUTURE_OPTION_HPP
