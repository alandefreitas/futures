//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_SRC_HPP
#define FUTURES_IMPL_SRC_HPP

// MUST COME FIRST
#ifndef FUTURES_SOURCE
#    define FUTURES_SOURCE
#endif

#include <futures/config.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
#    error Do not compile Futures library source with BOOST_ASIO_HEADER_ONLY defined
#endif

#if !defined(FUTURES_SEPARATE_COMPILATION)
#    error Do not compile Futures library source without FUTURES_SEPARATE_COMPILATION defined
#endif

// We always use compiled Asio when using compiled futures
#ifdef _WIN32
#    include <SDKDDKVer.h>
#endif
#if defined(FUTURES_USE_BUNDLED_ASIO) || defined(FUTURES_USE_STANDALONE_ASIO)
#    ifndef ASIO_SEPARATE_COMPILATION
#        define ASIO_SEPARATE_COMPILATION
#    endif
#elif defined(FUTURE_USE_BOOST)
#    ifndef BOOST_ASIO_SEPARATE_COMPILATION
#        define BOOST_ASIO_SEPARATE_COMPILATION
#    endif
#else
#    error Logic error. FUTURES_USE_XXXXXX_ASIO undefined.
#endif
#include <futures/detail/deps/asio/impl/src.hpp>

// #glob <futures/**.ipp> - <futures/detail/bundled/**.ipp>
#include <futures/executor/impl/default_executor.ipp>
#include <futures/executor/impl/inline_executor.ipp>
#include <futures/executor/impl/new_thread_executor.ipp>
#include <futures/impl/error.ipp>


#endif // FUTURES_IMPL_SRC_HPP
