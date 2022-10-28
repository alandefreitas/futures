//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_SRC_HPP
#define FUTURES_IMPL_SRC_HPP

#include <futures/config.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY) || !defined(FUTURES_SEPARATE_COMPILATION)
#    error Do not compile Futures library source with BOOST_ASIO_HEADER_ONLY defined
#endif

#if defined(FUTURES_USE_BUNDLED_ASIO) && defined(FUTURES_SEPARATE_COMPILATION)
#    ifdef _WIN32
#        include <SDKDDKVer.h>
#    endif
#    ifndef ASIO_SEPARATE_COMPILATION
#        define ASIO_SEPARATE_COMPILATION
#    endif
#    include <futures/detail/deps/asio/impl/src.hpp>
#endif

// #glob <futures/**.ipp> - <futures/detail/bundled/**.ipp>
#include <futures/executor/impl/default_executor.ipp>
#include <futures/executor/impl/inline_executor.ipp>
#include <futures/executor/impl/new_thread_executor.ipp>
#include <futures/impl/future_error.ipp>

#endif // FUTURES_IMPL_SRC_HPP
