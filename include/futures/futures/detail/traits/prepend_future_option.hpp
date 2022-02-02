//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_TRAITS_PREPEND_FUTURE_OPTION_HPP
#define FUTURES_FUTURES_DETAIL_TRAITS_PREPEND_FUTURE_OPTION_HPP

#include <futures/futures/future_options.hpp>

namespace futures::detail {
    template <class Opt, class Opts, class = void>
    struct prepend_future_option
    {
        using type = Opts;
    };

    template <class Opt, class... Args>
    struct prepend_future_option<
        Opt,
        future_options<Args...>,
        std::enable_if_t<!detail::is_in_args_v<Opt>>>
    {
        using type = future_options<Opt, Args...>;
    };

    template <class Opt, class Opts>
    using prepend_future_option_t = typename prepend_future_option<Opt, Opts>::
        type;

} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_TRAITS_PREPEND_FUTURE_OPTION_HPP
