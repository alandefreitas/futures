//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_THROW_EXCEPTION_HPP
#define FUTURES_DETAIL_THROW_EXCEPTION_HPP

#include <futures/detail/deps/boost/throw_exception.hpp>

namespace futures::detail {
    template <class E>
    BOOST_NORETURN void
    throw_exception(
        E&& e,
        boost::source_location const& loc = BOOST_CURRENT_LOCATION) {
#if BOOST_VERSION >= 107900
        boost::throw_with_location(std::forward<E>(e), loc);
#else
        boost::throw_exception(std::forward<E>(e), loc);
#endif
    }
} // namespace futures::detail

#endif // FUTURES_DETAIL_THROW_EXCEPTION_HPP
