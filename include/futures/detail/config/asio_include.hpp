//
// Copyright (c) Alan de Freitas 11/18/21.
//

#ifndef FUTURES_DETAIL_CONFIG_ASIO_INCLUDE_HPP
#define FUTURES_DETAIL_CONFIG_ASIO_INCLUDE_HPP

/// @file
/// Indirectly includes asio or boost.asio
///
/// Whenever including <asio.hpp>, we include this file instead.
/// This ensures the logic of including asio or boost::asio is consistent and
/// that we never include both.
///
/// Because this is not a networking library, at this point, we only depend on
/// the asio execution concepts (which we can forward-declare) and its
/// thread-pool, which is not very advanced. So this means we might be able to
/// remove boost asio as a dependency at some point and, because the small
/// vector library is also not mandatory, we can make this library free of
/// dependencies.
///

#ifdef _WIN32
#    include <SDKDDKVer.h>
#endif

/*
 * Check what versions of asio are available.
 *
 * We use __has_include<...> as a first alternative. If this fails,
 * we use some common assumptions.
 *
 */
#if defined __has_include
#    if __has_include(<asio.hpp>)
#        define FUTURES_HAS_ASIO
#    endif
#endif

#if defined __has_include
#    if __has_include(<boost/asio.hpp>)
#        define FUTURES_HAS_BOOST_ASIO
#    endif
#endif

// Recur to simple assumptions when not available.
#if !defined(FUTURES_HAS_BOOST_ASIO) && !defined(FUTURES_HAS_ASIO)
#    if FUTURES_PREFER_BOOST_DEPENDENCIES // user prefers boost -> assume boost
#        define FUTURES_HAS_BOOST_ASIO
#    elif FUTURES_PREFER_STANDALONE_DEPENDENCIES // user prefers asio -> asio
#        define FUTURES_HAS_ASIO
#    elif ASIO_STANDALONE // standalone is included -> assume standalone
#        define FUTURES_HAS_ASIO
#    elif BOOST_CXX_VERSION // boost is included -> assume boost
#        define FUTURES_HAS_BOOST_ASIO
#    elif FUTURES_STANDALONE // futures is standalone -> assume standalone
#        define FUTURES_HAS_ASIO
#    else // otherwise -> assume boost
#        define FUTURES_HAS_BOOST_ASIO
#    endif
#endif

/*
 * Decide what version of asio to use
 */

// If the standalone is available, this is what we assume the user usually
// prefers, since it's more specific
#if defined(FUTURES_HAS_ASIO)           \
    && !(                               \
        defined(FUTURES_HAS_BOOST_ASIO) \
        && defined(FUTURES_PREFER_BOOST_DEPENDENCIES))
#    define FUTURES_USE_ASIO
#    include <asio.hpp>
#else
#    define FUTURES_USE_BOOST_ASIO
#    include <boost/asio.hpp>
#endif

/*
 * Create the aliases
 */

namespace futures {
    /** @addtogroup executors Executors
     *  @{
     */

#if defined(FUTURES_USE_ASIO) || defined(FUTURES_DOXYGEN)
    /// Alias to the asio namespace
    ///
    /// This futures::asio alias might point to ::asio or ::boost::asio.
    ///
    /// If you are referring to the asio namespace and need it
    /// to match the namespace used by futures, prefer `futures::asio`
    /// instead of using `::asio` or `::boost::asio` directly.
    namespace asio = ::asio;
#else
    namespace asio = ::boost::asio;
#endif
    /** @} */
} // namespace futures

#endif // FUTURES_DETAIL_CONFIG_ASIO_INCLUDE_HPP
