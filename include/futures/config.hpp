//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_CONFIG_HPP
#define FUTURES_CONFIG_HPP

/** @file config.hpp
 *  @brief Public configuration macros
 *
 *  This file includes public configuration macros. These are the macros
 *  the user is allowed to define to change how the library is compiled.
 */

#include <futures/detail/config.hpp>

/// Macro used to indicate standalone asio is available
/**
 * This macro can be defined to indicate the standalone version of Asio is
 * available.
 *
 * If both the standalone and the Boost versions of Asio are available, the
 * standalone version is preferred unless @ref FUTURES_PREFER_BOOST is defined.
 *
 * If both the standalone and the Boost versions of Asio are unavailable, a
 * bundled version of the subset of Asio required by library is used.
 *
 * @par Default value
 * In C++17, this macro is determined by the availability of the `asio.hpp`
 * header.
 *
 * When using the CMake package, the `futures` target will already define this
 * macro if appropriate.
 *
 * In all other cases, the macro is undefined by default. If standalone Asio is
 * available, it should be defined manually.
 *
 * @par References
 * @li <a href="https://think-async.com/Asio/">Asio</a>
 *
 */
#if defined __has_include
#    if __has_include(<asio.hpp>)
#        define FUTURES_HAS_ASIO
#    endif
#elif ASIO_HAS_CONSTEXPR || FUTURES_DOXYGEN
#    define FUTURES_HAS_ASIO
#endif

/// Macro used to indicate Boost is available
/**
 * This macro can be defined to indicate that Boost is available as a
 * dependency.
 *
 * If both the standalone and the Boost versions of Asio are available, the
 * standalone version is preferred unless @ref FUTURES_PREFER_BOOST is defined.
 *
 * If both the standalone and the Boost versions of Asio are unavailable, a
 * bundled version of the subset of Boost required by library is used.
 *
 * @par Default value
 *
 * In C++17, this macro is determined by the availability of the `asio.hpp`
 * header.
 *
 * When using the CMake package, the `futures` target will already define this
 * macro if appropriate.
 *
 * In all other cases, the macro is undefined by default. If standalone Asio is
 * available, it should be defined manually.
 *
 * @par References
 * @li <a href="https://www.boost.org/">Boost</a>
 *
 */
#if defined __has_include
#    if __has_include(<boost/config.hpp>)
#        define FUTURES_HAS_BOOST
#    endif
#elif BOOST_USER_CONFIG || FUTURES_DOXYGEN
#    define FUTURES_HAS_BOOST
#endif

/// Macro used to indicate we prefer using standalone Asio over Boost.Asio
/**
 * This macro can be defined to indicate that we should prefer standalone
 * Asio over Boost whenever standalone Asio is available.
 *
 * If both the standalone and the Boost versions of Asio are available, this
 * macro ensure the standalone version is used.
 *
 * The availability of standalone Asio and Boost.Asio can be indicated with the
 * @ref FUTURES_HAS_ASIO and @ref FUTURES_HAS_BOOST macros.
 *
 * @par Default value
 *
 * In C++17, this macro is defined whenever @ref FUTURES_HAS_ASIO is defined
 * and @ref FUTURES_PREFER_BOOST is undefined.
 *
 * When using the CMake package, the `futures` target will already define this
 * macro when standalone Asio is available.
 *
 * In all other cases, the macro is undefined by default. If standalone Asio is
 * available, it should be defined manually.
 *
 * If both @ref FUTURES_HAS_ASIO and @ref FUTURES_HAS_BOOST are undefined,
 * a bundled subset of Boost dependencies required by the library is used.
 *
 * @par References
 * @li <a href="https://think-async.com/Asio/">Asio</a>
 * @li <a href="https://www.boost.org/">Boost</a>
 *
 */
#if !defined(FUTURES_PREFER_BOOST) && !defined(FUTURES_PREFER_ASIO)
#    ifdef FUTURES_HAS_ASIO
#        define FUTURES_PREFER_ASIO
#    else
#        define FUTURES_PREFER_BOOST
#    endif
#endif

/// @def FUTURES_PREFER_BOOST
/// Macro used to indicate we prefer using Boost.Asio over standalone Asio
/**
 * This macro can be defined to indicate that we should prefer Boost.Asio over
 * standalone Asio over Boost whenever standalone Boost is available.
 *
 * If both the standalone and the Boost versions of Asio are available, this
 * macro ensure the Boost version is used.
 *
 * The availability of standalone Asio and Boost.Asio can be indicated with the
 * @ref FUTURES_HAS_ASIO and @ref FUTURES_HAS_BOOST macros.
 *
 * @par Default value
 *
 * In C++17, this macro is defined whenever @ref FUTURES_HAS_BOOST is defined
 * and @ref FUTURES_PREFER_ASIO is undefined.
 *
 * When using the CMake package, the `futures` target will already define this
 * macro when standalone Asio is unavailable.
 *
 * In all other cases, the macro is undefined by default. If Boost is
 * available, it should be defined manually.
 *
 * If both @ref FUTURES_HAS_ASIO and @ref FUTURES_HAS_BOOST are undefined,
 * a bundled subset of Boost dependencies required by the library is used.
 *
 * @par References
 * @li <a href="https://think-async.com/Asio/">Asio</a>
 * @li <a href="https://www.boost.org/">Boost</a>
 *
 */
#ifdef FUTURES_DOXYGEN
#    define FUTURES_PREFER_BOOST
#endif

/// To make the library *not* header-only, the user can:
/// - create a source file,
/// - include this header in it,
/// - compile it,
/// - link it with the binary, and
/// - include the ASIO_SEPARATE_COMPILATION compile definition
///

/// @def FUTURES_SEPARATE_COMPILATION
/// Use separately compiled source code for implementation
/**
 * By default, Futures is a header-only library. To reduce compile times, users
 * can also build the library using separately compiled source code.
 *
 * If the library is integrated with CMake, the appropriate macros should
 * already define the appropriate macros for separate compilation.
 *
 * To do this without a build system, add `#include <futures/impl/src.hpp>`
 * to one (and only one) source file of your program, then build the program
 * with `FUTURES_SEPARATE_COMPILATION` defined in the project\/compiler
 * settings.
 *
 * When using this library with Asio, this option is independent of the
 * `ASIO_SEPARATE_COMPILATION` or `BOOST_ASIO_SEPARATE_COMPILATION` options.
 * `FUTURES_SEPARATE_COMPILATION` only implies in
 * `BOOST_ASIO_SEPARATE_COMPILATION` when both standalone Asio and Boost.Asio
 * are unavailable and the bundled version of Asio is used.
 *
 */
#ifdef FUTURES_DOXYGEN
#    define FUTURES_SEPARATE_COMPILATION
#endif

#include <futures/impl/config.hpp>

#endif // FUTURES_CONFIG_HPP
