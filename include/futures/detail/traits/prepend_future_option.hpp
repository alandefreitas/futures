//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_PREPEND_FUTURE_OPTION_HPP
#define FUTURES_DETAIL_TRAITS_PREPEND_FUTURE_OPTION_HPP

#include <futures/detail/future_options_list.hpp>

namespace futures {
    namespace detail {
        template <class Opt, class Opts, class = void>
        struct prepend_future_option {
            using type = Opts;
        };

        template <class Opt, class... Args>
        struct prepend_future_option<
            Opt,
            future_options_list<Args...>,
            std::enable_if_t<!mp_contains<mp_list<Args...>, Opt>::value>> {
            using type = future_options_list<Opt, Args...>;
        };

        template <class Opt, class Opts>
        using prepend_future_option_t =
            typename prepend_future_option<Opt, Opts>::type;

    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_TRAITS_PREPEND_FUTURE_OPTION_HPP
