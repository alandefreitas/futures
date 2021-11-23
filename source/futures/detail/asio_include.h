//
// Created by alandefreitas on 11/18/21.
//

#ifndef FUTURES_ASIO_INCLUDE_H
#define FUTURES_ASIO_INCLUDE_H

/// \file Whenever including <asio.hpp>, we include this file instead
/// This ensures the logic of including asio or boost::asio is consistent

#ifdef _WIN32
#include <SDKDDKVer.h>
#endif

/*
 * Check what versions of asio are available
 *
 * We use __has_include<...> because we are targeting C++17
 *
 */
#if __has_include(<asio.hpp>)
#define FUTURES_HAS_ASIO
#endif

#if __has_include(<boost/asio.hpp>)
#define FUTURES_HAS_BOOST_ASIO
#endif

#if !defined(FUTURES_HAS_BOOST_ASIO) && !defined(FUTURES_HAS_ASIO)
#error Asio headers not found
#endif

/*
 * Decide what version of asio to use
 */

// If the standalone is available, this is what we assume the user usually prefers, since it's more specific
#if defined(FUTURES_HAS_ASIO) && !(defined(FUTURES_HAS_BOOST_ASIO) && defined(FUTURES_PREFER_BOOST_DEPENDENCIES))
#define FUTURES_USE_ASIO
#include <asio.hpp>
#else
#define FUTURES_USE_BOOST_ASIO
#include <boost/asio.hpp>
#endif

/*
 * Create the aliases
 */

namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

#if defined(FUTURES_USE_ASIO) || defined(FUTURES_DOXYGEN)
    /// \brief Alias to the asio namespace
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
}

#endif // FUTURES_ASIO_INCLUDE_H
