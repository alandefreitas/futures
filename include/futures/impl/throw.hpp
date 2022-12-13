//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_THROW_HPP
#define FUTURES_IMPL_THROW_HPP

#include <futures/throw.hpp>
#include <futures/detail/deps/boost/throw_exception.hpp>

namespace futures {
    template <class E>
    BOOST_NORETURN
    void
    throw_exception(E&& e, source_location const& loc) {
        // By default, we forward the exception to boost.throw_exception
        // If exceptions are disabled, boost.throw_exception will call the
        // custom
#if BOOST_VERSION >= 107900
        boost::throw_with_location(std::forward<E>(e), loc);
#else
        boost::throw_exception(std::forward<E>(e), loc);
#endif
    }
} // namespace futures

#endif // FUTURES_IMPL_THROW_HPP
