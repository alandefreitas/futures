//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_CONFIG_HPP
#define FUTURES_IMPL_CONFIG_HPP

/// @file
/// Apply config macros
/**
 * This file applies the public configuration macros.
 *
 * This involves defining private shortcuts that depend on the existing
 * public macros and raising the namespaces that are valid for all headers
 * in the library.
 */

/*
 * Decide what version of asio to use
 *
 * If the standalone is available, this is what we assume the user usually
 * prefers, since it's more specific
 */
#if defined(FUTURES_HAS_ASIO) \
    && (!defined(FUTURES_HAS_BOOST) || !defined(FUTURES_PREFER_BOOST))
#    define FUTURES_USE_STANDALONE_ASIO
#elif defined(FUTURES_HAS_BOOST)
#    define FUTURES_USE_BOOST_ASIO
#else
#    define FUTURES_USE_BUNDLED_ASIO
#endif

/*
 * Set boost/user if there's no boost
 */
#if defined FUTURES_USE_BUNDLED_ASIO && !defined BOOST_USER_CONFIG
#    define BOOST_USER_CONFIG <futures/detail/bundled/boost/config/user.hpp>
#endif

/*
 * Define namespaces
 *
 * We raise and create aliases for the asio or boost asio namespaces,
 * depending on what library we should use.
 */
#ifdef FUTURES_USE_STANDALONE_ASIO
namespace asio {}
#else
namespace boost {
    namespace asio {}
} // namespace boost
#endif

// Include OS headers
#ifdef _WIN32
#    include <SDKDDKVer.h>
#endif

// Identify if the library is header-only
#ifndef FUTURES_HEADER_ONLY
#    ifndef FUTURES_SEPARATE_COMPILATION
#        define FUTURES_HEADER_ONLY
#    endif
#endif

/*
 * Also raise mp11 library namespace
 */
namespace boost {
    namespace mp11 {}
} // namespace boost

namespace futures {
    /*
     * Aliases
     */
#if defined(FUTURES_DOXYGEN)
    namespace asio = __see_below__;
#elif defined(FUTURES_USE_STANDALONE_ASIO)
    namespace asio = ::asio;
#else
    namespace asio = ::boost::asio;
#endif

    namespace detail {
        using namespace boost::mp11;
    } // namespace detail
} // namespace futures

#endif // FUTURES_IMPL_CONFIG_HPP
