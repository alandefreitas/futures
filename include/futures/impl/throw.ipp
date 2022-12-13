//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_THROW_IPP
#define FUTURES_IMPL_THROW_IPP

#include <futures/config.hpp>
#include <futures/throw.hpp>
#ifndef FUTURES_CUSTOM_EXCEPTION_HANDLE
#    include <exception>
#endif

#ifndef FUTURES_CUSTOM_EXCEPTION_HANDLE
namespace futures {
    void
    throw_exception(std::exception const&, boost::source_location const&) {
        std::terminate();
    }
} // namespace futures
#endif

#ifdef BOOST_NO_EXCEPTIONS
namespace boost {
    void
    throw_exception(std::exception const& ex) {
        futures::throw_exception(ex, BOOST_CURRENT_LOCATION);
    }

    void
    throw_exception(
        std::exception const& ex,
        boost::source_location const& loc) {
        futures::throw_exception(ex, loc);
    }
} // namespace boost
#endif

#endif // FUTURES_IMPL_THROW_IPP
