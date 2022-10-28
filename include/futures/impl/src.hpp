//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_SRC_HPP
#define FUTURES_IMPL_SRC_HPP

#include <futures/config.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
#    error Do not compile Futures library source with BOOST_ASIO_HEADER_ONLY defined
#endif

#if defined(FUTURES_USE_BUNDLED_ASIO) && defined(FUTURES_SEPARATE_COMPILATION)
#    ifdef _WIN32
#        include <SDKDDKVer.h>
#    endif
#    define ASIO_SEPARATE_COMPILATION
#    include <futures/detail/deps/asio/impl/src.hpp>
#endif

// #glob <futures/**.ipp> - <futures/detail/bundled/**.ipp>


#endif // FUTURES_IMPL_SRC_HPP
