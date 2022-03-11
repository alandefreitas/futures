//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_TRAITS_APPEND_FUTURE_OPTION_HPP
#define FUTURES_FUTURES_DETAIL_TRAITS_APPEND_FUTURE_OPTION_HPP

#include <futures/futures/detail/future_options_list.hpp>

namespace futures::detail {
    template <class Opt, class Opts, class = void>
    struct append_future_option
    {
        using type = Opts;
    };

    template <class Opt, class... Args>
    struct append_future_option<
        Opt,
        future_options_list<Args...>,
        std::enable_if_t<!detail::is_in_args_v<Opt, Args...>>>
    {
        using type = future_options_list<Args..., Opt>;
    };

    template <class Opt, class Opts>
    using append_future_option_t = typename append_future_option<Opt, Opts>::
        type;

    template <bool B, class Opt, class Opts>
    using conditional_append_future_option = std::
        conditional<B, append_future_option_t<Opt, Opts>, Opts>;

    template <bool B, class Opt, class Opts>
    using conditional_append_future_option_t =
        typename conditional_append_future_option<B, Opt, Opts>::type;


} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_TRAITS_APPEND_FUTURE_OPTION_HPP
