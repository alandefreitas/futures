//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_CONFIG_HPP
#define FUTURES_IMPL_CONFIG_HPP

/*
 * Apply configuration macros
 *
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
#  define BOOST_USER_CONFIG <futures/detail/deps/boost/config/user.hpp> // redirect boost include
#endif

/*
 * Define namespaces
 *
 * We raise and create aliases for the asio or boost asio namespaces,
 * depending on what library we should use.
 */
#ifdef FUTURES_USE_BOOST_ASIO
namespace boost {
    namespace asio {}
} // namespace boost
#else
namespace asio {}
#endif

// Include OS headers
#ifdef _WIN32
#    include <SDKDDKVer.h>
#endif

// Identify if the library is header-only
#if !defined(FUTURES_HEADER_ONLY) && !defined(FUTURES_SEPARATE_COMPILATION)
#    define FUTURES_HEADER_ONLY
#endif

#if !defined(FUTURES_DOXYGEN)
/*
 * Also raise mp11 library namespace
 */
namespace boost {
    namespace mp11 {}
} // namespace boost
#endif

namespace futures {
    /*
     * Aliases
     */
#if defined(FUTURES_DOXYGEN)
    namespace asio = __see_below__;
#elif defined(FUTURES_USE_BOOST_ASIO)
    namespace asio = ::boost::asio;
#else
    namespace asio = ::asio;
#endif

    namespace detail {
        using namespace boost::mp11;
    } // namespace detail
} // namespace futures

#endif // FUTURES_IMPL_CONFIG_HPP
