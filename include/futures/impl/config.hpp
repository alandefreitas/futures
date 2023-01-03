//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_CONFIG_HPP
#define FUTURES_IMPL_CONFIG_HPP

#include <futures/detail/deps/boost/config.hpp>

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
 * Determine what versions of asio are available
 */
#if defined __has_include
#    if __has_include(<asio.hpp>)
#        ifndef FUTURES_HAS_STANDALONE_ASIO
#            define FUTURES_HAS_STANDALONE_ASIO
#        endif
#    endif
#elif ASIO_HAS_CONSTEXPR
#    define FUTURES_HAS_STANDALONE_ASIO
#endif

#if defined __has_include
#    if __has_include(<boost/config.hpp>)
#        ifndef FUTURES_HAS_BOOST
#            define FUTURES_HAS_BOOST
#        endif
#    endif
#elif defined(BOOST_USER_CONFIG)
#    define FUTURES_HAS_BOOST
#endif


/*
 * Determine what version of asio to use
 *
 * If the standalone is available, this is what we assume the user usually
 * prefers, since it's more specific
 */
#if defined(FUTURES_HAS_STANDALONE_ASIO)                               \
    && (!defined(FUTURES_HAS_BOOST) || !defined(FUTURES_PREFER_BOOST)) \
    && !defined(FUTURES_PREFER_BUNDLED)
#    ifndef FUTURES_USE_STANDALONE_ASIO
#        define FUTURES_USE_STANDALONE_ASIO
#    endif
#elif defined(FUTURES_HAS_BOOST) && !defined(FUTURES_PREFER_BUNDLED)
#    ifndef FUTURES_USE_BOOST_ASIO
#        define FUTURES_USE_BOOST_ASIO
#    endif
#else
#    ifndef FUTURES_USE_BUNDLED_ASIO
#        define FUTURES_USE_BUNDLED_ASIO
#    endif
#endif

/*
 * Set boost/user if there's no boost
 */
#if defined(FUTURES_USE_BUNDLED_ASIO) && !defined(BOOST_USER_CONFIG)
#    define BOOST_USER_CONFIG <futures/detail/deps/boost/config/user.hpp>
#endif

/*
 * Determine separate compilation and header only flags
 *
 * If the standalone is available, this is what we assume the user usually
 * prefers, since it's more specific
 */
// Include OS headers
#ifdef _WIN32
#    include <SDKDDKVer.h>
#endif

// Identify if the library is header-only
#if !defined(FUTURES_HEADER_ONLY) && !defined(FUTURES_SEPARATE_COMPILATION)
#    define FUTURES_HEADER_ONLY
#endif

// Avoid including Asio symbols twice
#ifdef FUTURES_SEPARATE_COMPILATION
#    if defined(FUTURES_USE_STANDALONE_ASIO) \
        || defined(FUTURES_USE_BUNDLED_ASIO)
#        ifndef ASIO_SEPARATE_COMPILATION
#            define ASIO_SEPARATE_COMPILATION
#        endif
#    endif
#    if defined(FUTURES_USE_BOOST_ASIO)
#        ifndef BOOST_ASIO_SEPARATE_COMPILATION
#            define BOOST_ASIO_SEPARATE_COMPILATION
#        endif
#    endif
#endif

// Macro to declare functions
#ifdef FUTURES_HEADER_ONLY
#    define FUTURES_DECLARE inline
#else
#    if defined(FUTURES_DYN_LINK) && !defined(FUTURES_STATIC_LINK)
#        if defined(FUTURES_SOURCE)
#            define FUTURES_DECLARE BOOST_SYMBOL_EXPORT
#        else
#            define FUTURES_DECLARE BOOST_SYMBOL_IMPORT
#        endif
#    endif
#endif
#ifndef FUTURES_DECLARE
#    define FUTURES_DECLARE
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
#if defined(FUTURES_USE_BOOST_ASIO)
    namespace asio = FUTURES_DETAIL(::boost::asio);
#else
    namespace asio = FUTURES_DETAIL(::asio);
#endif

    namespace detail {
        using namespace boost::mp11;
    } // namespace detail
} // namespace futures

/*
 * Exceptions
 */
#ifndef FUTURES_NO_EXCEPTIONS
#    ifdef BOOST_NO_EXCEPTIONS
#        define FUTURES_NO_EXCEPTIONS
#    endif
#endif


#endif // FUTURES_IMPL_CONFIG_HPP
