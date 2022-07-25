//
// Copyright (c) Alan de Freitas 11/18/21.
//

#ifndef FUTURES_DETAIL_CONFIG_HPP
#define FUTURES_DETAIL_CONFIG_HPP

#ifdef _WIN32
#    include <SDKDDKVer.h>
#endif

/*
 * Detect Standalone ASIO
 */

#if ASIO_HAS_CONSTEXPR // if standalone asio already included
#    define FUTURES_HAS_ASIO
#elif defined __has_include // if main asio header is available
#    if __has_include(<asio.hpp>)
#        define FUTURES_HAS_ASIO
#    endif
#endif
#ifndef FUTURES_HAS_ASIO // use fake header to detect it
#    include <futures/detail/detect/asio.hpp>
#    ifndef FUTURES_ASIO_NOT_FOUND
#        define FUTURES_HAS_ASIO
#    endif
#endif

/*
 * Detect Boost
 */

#if BOOST_USER_CONFIG // if boost is already included
#    define FUTURES_HAS_BOOST
#elif defined __has_include // if boost config header is available
#    if __has_include(<boost/config.hpp>)
#        define FUTURES_HAS_BOOST
#    endif
#endif
#ifndef FUTURES_HAS_BOOST // use fake header to detect it
#    include <futures/detail/detect/boost.hpp>
#    ifndef FUTURES_BOOST_NOT_FOUND
#        define FUTURES_HAS_BOOST
#    endif
#endif

/*
 * Define preference between boost.asio and asio
 */

#if !defined(FUTURES_PREFER_BOOST) && !defined(FUTURES_PREFER_ASIO)
#    ifdef FUTURES_HAS_ASIO
#        define FUTURES_PREFER_ASIO
#    else
#        define FUTURES_PREFER_BOOST
#    endif
#endif

/*
 * Decide what version of asio to use
 *
 * If the standalone is available, this is what we assume the user usually
 * prefers, since it's more specific
 */
#if defined(FUTURES_HAS_ASIO) \
    && (!defined(FUTURES_HAS_BOOST) || !defined(FUTURES_PREFER_BOOST))
#    define FUTURES_USE_STANDALONE_ASIO
namespace asio {}
#elif defined(FUTURES_HAS_BOOST)
#    define FUTURES_USE_BOOST_ASIO
namespace boost {
    namespace asio {}
} // namespace boost
#else
#    define FUTURES_USE_BUNDLED_ASIO
namespace boost {
    namespace asio {}
} // namespace boost
#endif


namespace futures {
    /** @addtogroup aliases Aliases
     *  @{
     */

    /// Alias to the asio namespace
    /**
     *  This futures::asio alias might point to ::asio or ::boost::asio.
     *
     *  If you are referring to the asio namespace and need it
     *  to match the namespace used by futures, prefer `futures::asio`
     *  instead of using `::asio` or `::boost::asio` directly.
     */
#if defined(FUTURES_DOXYGEN)
    namespace asio = __see_below__;
#elif defined(FUTURES_USE_STANDALONE_ASIO)
    namespace asio = ::asio;
#else
    namespace asio = ::boost::asio;
#endif
    /** @} */
} // namespace futures

#endif // FUTURES_DETAIL_CONFIG_HPP
